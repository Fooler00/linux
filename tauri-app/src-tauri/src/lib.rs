use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command as StdCommand;
use std::sync::Mutex;
use std::thread;
use std::time::Duration;

use reqwest::Method;
use reqwest::multipart;
use tauri::{Emitter, Manager, RunEvent};
use tauri_plugin_shell::process::{CommandChild, CommandEvent};
use tauri_plugin_shell::ShellExt;

const API_BASE: &str = "http://127.0.0.1:8080";
const BACKUP_PORT: &str = "8080";
const BACKUP_LOCK_PATH: &str = "/tmp/backup_server_8080.lock";

struct BackupServerState {
    child: Mutex<Option<CommandChild>>,
}

fn kill_managed_sidecar(app: &tauri::AppHandle) {
    if let Some(state) = app.try_state::<BackupServerState>() {
        if let Ok(mut guard) = state.child.lock() {
            if let Some(child) = guard.take() {
                let pid = child.pid();
                if let Err(err) = child.kill() {
                    eprintln!("[backup_server] kill failed (pid={pid}): {err}");
                } else {
                    eprintln!("[backup_server] killed on app exit (pid={pid})");
                }
            }
        }
    }
}

/// 清理上次异常退出残留的 backup_server（与 C++ 单实例锁文件约定一致）。
fn cleanup_stale_backup_server() {
    let Ok(contents) = fs::read_to_string(BACKUP_LOCK_PATH) else {
        return;
    };
    let Some(pid_str) = contents
        .lines()
        .next()
        .map(str::trim)
        .filter(|s| !s.is_empty())
    else {
        return;
    };
    let Ok(pid) = pid_str.parse::<u32>() else {
        return;
    };

    if !is_backup_server_pid(pid) {
        return;
    }

    eprintln!("[backup_server] cleaning stale process pid={pid}");
    let _ = StdCommand::new("kill")
        .args(["-TERM", &pid.to_string()])
        .status();
    thread::sleep(Duration::from_millis(300));
    if PathBuf::from(format!("/proc/{pid}")).exists() {
        let _ = StdCommand::new("kill")
            .args(["-KILL", &pid.to_string()])
            .status();
        thread::sleep(Duration::from_millis(200));
    }
}

fn is_backup_server_pid(pid: u32) -> bool {
    if let Ok(cmdline) = fs::read(format!("/proc/{pid}/cmdline")) {
        if String::from_utf8_lossy(&cmdline).contains("backup_server") {
            return true;
        }
    }
    if let Ok(comm) = fs::read_to_string(format!("/proc/{pid}/comm")) {
        return comm.trim() == "backup_server";
    }
    false
}

async fn wait_for_server() -> Result<(), String> {
    let client = reqwest::Client::new();

    for _ in 0..60 {
        if client
            .get(format!("{API_BASE}/api/tasks"))
            .send()
            .await
            .map(|r| r.status().is_success())
            .unwrap_or(false)
        {
            return Ok(());
        }

        tokio::time::sleep(Duration::from_millis(500)).await;
    }

    Err("备份服务启动超时".into())
}

fn spawn_backup_server(app: &tauri::AppHandle) -> Result<(), String> {
    cleanup_stale_backup_server();

    let runtime_dir = resolve_runtime_dir(app)?;
    let cloud_dir = runtime_dir.join("cloud_storage");
    let db_path = runtime_dir.join("users.db");

    let sidecar = app
        .shell()
        .sidecar("backup_server")
        .map(|command| {
            command
                .current_dir(&runtime_dir)
                .env("BACKUP_PORT", BACKUP_PORT)
                .env("BACKUP_DB", db_path.as_os_str())
                .env("BACKUP_CLOUD_DIR", cloud_dir.as_os_str())
                .env("BACKUP_CLOUD_TYPE", "local")
        })
        .map_err(|e| format!("无法加载 sidecar: {e}"))?;

    let (mut rx, child) = sidecar
        .spawn()
        .map_err(|e| format!("无法启动备份服务: {e}"))?;

    app.manage(BackupServerState {
        child: Mutex::new(Some(child)),
    });

    let app_handle = app.clone();
    tauri::async_runtime::spawn(async move {
        while let Some(event) = rx.recv().await {
            if let CommandEvent::Stderr(line) = event {
                eprintln!(
                    "[backup_server] {}",
                    String::from_utf8_lossy(&line).trim_end()
                );
            } else if let CommandEvent::Error(err) = event {
                eprintln!("[backup_server] error: {err}");
            } else if let CommandEvent::Terminated(payload) = event {
                eprintln!(
                    "[backup_server] terminated: code={:?}",
                    payload.code
                );
                // 进程已退出时清空句柄，避免退出时对已死进程再 kill。
                if let Some(state) = app_handle.try_state::<BackupServerState>() {
                    if let Ok(mut guard) = state.child.lock() {
                        guard.take();
                    }
                }
                let _ = app_handle.emit("backup-server-stopped", ());
            }
        }
    });

    Ok(())
}

fn resolve_runtime_dir(app: &tauri::AppHandle) -> Result<PathBuf, String> {
    let runtime_dir = app
        .path()
        .app_data_dir()
        .map_err(|e| format!("无法解析应用数据目录: {e}"))?;

    fs::create_dir_all(runtime_dir.join("cloud_storage"))
        .map_err(|e| format!("无法创建运行时目录: {e}"))?;

    Ok(runtime_dir)
}

fn apply_auth_header(
    request: reqwest::RequestBuilder,
    auth_token: Option<String>,
    cloud_token: Option<String>,
    path: &str,
) -> reqwest::RequestBuilder {
    if path.starts_with("/api/cloud") {
        if let Some(token) = cloud_token.filter(|value| !value.trim().is_empty()) {
            return request.header("Authorization", format!("Bearer {token}"));
        }
    }

    if let Some(token) = auth_token.filter(|value| !value.trim().is_empty()) {
        return request.header("Authorization", format!("Bearer {token}"));
    }

    request
}

#[tauri::command]
async fn api_request(
    method: String,
    path: String,
    body: Option<String>,
    query: Option<HashMap<String, String>>,
    auth_token: Option<String>,
    cloud_token: Option<String>,
) -> Result<String, String> {
    let client = reqwest::Client::new();
    let method = Method::from_bytes(method.as_bytes()).map_err(|e| e.to_string())?;

    let mut request = client.request(method, format!("{API_BASE}{path}"));

    if let Some(params) = query {
        request = request.query(&params);
    }

    request = apply_auth_header(request, auth_token, cloud_token, &path);

    if let Some(payload) = body {
        request = request
            .header("Content-Type", "application/json")
            .body(payload);
    }

    let response = request.send().await.map_err(|e| e.to_string())?;
    let status = response.status();
    let text = response.text().await.map_err(|e| e.to_string())?;

    if status.is_success() {
        Ok(text)
    } else {
        Err(text)
    }
}

#[tauri::command]
async fn cloud_upload(
    local_path: String,
    remote_path: String,
    cloud_token: Option<String>,
) -> Result<String, String> {
    let local = Path::new(&local_path);
    if !local.is_file() {
        return Err("本地文件不存在".into());
    }

    let file_name = local
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or("upload.bin")
        .to_string();
    let file_bytes = fs::read(local).map_err(|e| format!("读取本地文件失败: {e}"))?;

    let part = multipart::Part::bytes(file_bytes)
        .file_name(file_name)
        .mime_str("application/octet-stream")
        .map_err(|e| e.to_string())?;

    let form = multipart::Form::new()
        .text("remotePath", remote_path)
        .part("file", part);

    let client = reqwest::Client::new();
    let mut request = client
        .post(format!("{API_BASE}/api/cloud/upload"))
        .multipart(form);

    if let Some(token) = cloud_token.filter(|value| !value.trim().is_empty()) {
        request = request.header("Authorization", format!("Bearer {token}"));
    }

    let response = request.send().await.map_err(|e| e.to_string())?;
    let status = response.status();
    let text = response.text().await.map_err(|e| e.to_string())?;

    if status.is_success() {
        Ok(text)
    } else {
        Err(text)
    }
}

#[tauri::command]
async fn cloud_download(
    remote_path: String,
    local_path: String,
    cloud_token: Option<String>,
) -> Result<String, String> {
    let client = reqwest::Client::new();
    let mut request = client
        .get(format!("{API_BASE}/api/cloud/download"))
        .query(&[("remotePath", remote_path.as_str())]);

    if let Some(token) = cloud_token.filter(|value| !value.trim().is_empty()) {
        request = request.header("Authorization", format!("Bearer {token}"));
    }

    let response = request.send().await.map_err(|e| e.to_string())?;
    let status = response.status();

    if !status.is_success() {
        let text = response.text().await.unwrap_or_default();
        return Err(if text.is_empty() {
            format!("下载失败，HTTP {}", status)
        } else {
            text
        });
    }

    let bytes = response.bytes().await.map_err(|e| e.to_string())?;
    let target = PathBuf::from(&local_path);

    if let Some(parent) = target.parent() {
        fs::create_dir_all(parent).map_err(|e| format!("创建目录失败: {e}"))?;
    }

    fs::write(&target, bytes).map_err(|e| format!("写入本地文件失败: {e}"))?;
    Ok(local_path)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
            spawn_backup_server(app.handle())?;
            let handle = app.handle().clone();

            tauri::async_runtime::block_on(async move {
                wait_for_server().await.map_err(|e| {
                    Box::new(std::io::Error::new(std::io::ErrorKind::TimedOut, e))
                        as Box<dyn std::error::Error>
                })
            })?;

            let _ = handle.emit("backup-server-ready", ());
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![api_request, cloud_upload, cloud_download])
        .build(tauri::generate_context!())
        .expect("error while building tauri application")
        .run(|app_handle, event| match event {
            RunEvent::ExitRequested { .. } | RunEvent::Exit => {
                kill_managed_sidecar(&app_handle);
            }
            _ => {}
        });
}
