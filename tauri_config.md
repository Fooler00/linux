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