参考环境：opensuse

## 安装系统依赖

```bash
sudo zypper in -t pattern devel_basis
sudo zypper in webkit2gtk3-soup2-devel libopenssl-devel curl wget file libappindicator3-1 librsvg-devel
```

## 安装rust环境

使用官方`rustup`安装。

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

显示系统已安装rust。先卸载系统rust。

锁定rust相关rpm包，将其卸载：

```bash
$ rpm -qa | grep -E 'rust|cargo'
cargo1.94-1.94.1-1.3.x86_64
rust1.94-1.94.1-1.3.x86_64
$ sudo zypper remove rust1.94 cargo1.94
```

选择默认安装。使环境变量生效。

```bash
source $HOME/.cargo/env
```

## 安装node.js

## 创建Tauri项目

```bash
npm create tauri-app@latest
```

创建项目完成后，提示缺少依赖：

```text
Template created!

Your system is missing dependencies (or they do not exist in $PATH):
╭────────────┬─────────────────────────────────────────────────────╮
│ webkit2gtk │ Visit https://tauri.app/guides/prerequisites/#linux │
╰────────────┴─────────────────────────────────────────────────────╯

Make sure you have installed the prerequisites for your OS: https://tauri.app/start/prerequisites/, then run:
  cd tauri-app
  npm install
  npm run tauri android init

For Desktop development, run:
  npm run tauri dev

For Android development, run:
  npm run tauri android dev
```

安装缺少的依赖：

```bash
zypper search webkit
sudo zypper install webkitgtk3-devel
```

安装项目依赖

```bash
npm install
```

运行：

```bash
npm run tauri dev
```

## 关于首次运行页面正常，但后续运行页面空白问题

在空白界面右键选择“重新加载”，可恢复正常。

### 原因

首次运行时，rust核心重新编译，耗时巨大；在此期间vite启动完成，等待连接。  
后续运行，rust核心增量编译，耗时较短。vite未准备好热更新服务与内存索引文件。tauri请求到空响应，或者webkit底层由于超时直接放弃后续加载。

### troubleshooting

#### 修改rust配置

在`tauri.conf.json`中添加以下配置:

```json
"visible": false
```

使得窗口启动时隐藏。

修改lib.rs文件，添加以下代码:

```rust
use tauri::Manager;

tauri::Builder::default()
    .plugin(tauri_plugin_opener::init())
    .invoke_handler(tauri::generate_handler![greet])
    .on_page_load(|window, _payload| {
        // 确保只对主窗口进行操作
        if window.label() == "main" {
            // 当前端页面完全加载完后，再解除隐身
            window.show().unwrap();
        }
    })
```

该方案失败。

#### 安装wait-on依赖

```bash
npm install -D wait-on
```

在tauri.conf.json中修改build.beforeDevCommand字段为：

```json
"beforeDevCommand": "npm run dev & npx wait-on http://127.0.0.1:1420",
```

锁定vite主机地址，防止tauri读取ipv4地址。

修改vite.config.ts文件，将动态解析端口改为固定端口。