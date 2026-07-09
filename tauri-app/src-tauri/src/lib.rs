use std::collections::HashMap;
use std::time::Duration;

use reqwest::Method;
use tauri::Emitter;
use tauri_plugin_shell::process::CommandEvent;
use tauri_plugin_shell::ShellExt;

const API_BASE: &str = "http://127.0.0.1:8080";

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
    let sidecar = app
        .shell()
        .sidecar("backup_server")
        .map_err(|e| format!("无法加载 sidecar: {e}"))?;

    let (mut rx, _child) = sidecar
        .spawn()
        .map_err(|e| format!("无法启动备份服务: {e}"))?;

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
                let _ = app_handle.emit("backup-server-stopped", ());
            }
        }
    });

    Ok(())
}

#[tauri::command]
async fn api_request(
    method: String,
    path: String,
    body: Option<String>,
    query: Option<HashMap<String, String>>,
    auth_token: Option<String>,
) -> Result<String, String> {
    let client = reqwest::Client::new();
    let method = Method::from_bytes(method.as_bytes()).map_err(|e| e.to_string())?;

    let mut request = client.request(method, format!("{API_BASE}{path}"));

    if let Some(params) = query {
        request = request.query(&params);
    }

    if let Some(token) = auth_token.filter(|value| !value.trim().is_empty()) {
        request = request.header("Authorization", format!("Bearer {token}"));
    }

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

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
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
        .invoke_handler(tauri::generate_handler![api_request])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
