# 数据备份软件（Data Backup）

**课程：** 软件开发综合实验  
**项目：** 桌面端数据备份工具  

本项目实现一套可在 Linux 桌面运行的数据备份软件：前端基于 **Vue 3 + TypeScript + Tauri 2**，后端核心为 **C++** 服务（以 Sidecar 方式随应用启动），支持手动备份、还原、实时监听、定时备份、备份管理等功能。

---

## 功能概览

| 模块 | 说明 |
|------|------|
| 用户认证 | 注册 / 登录 |
| 手动备份 | 单文件、多文件、目录；可选归档（zip / tar / tar.gz）、加密、增量、高级筛选 |
| 备份还原 | 从备份路径还原到指定目录（加密备份需密码） |
| 实时监听 | 监视源目录变更并自动备份 |
| 定时备份 | 按间隔自动备份，支持保留策略 |
| 备份管理 | 查询历史备份、查看元数据、按数量/天数淘汰 |
| 任务列表 | 展示备份 / 还原等任务状态 |

---

## 技术栈

- **前端：** Vue 3、TypeScript、Vite、Tauri 2  
- **后端：** C++（HTTP API + 备份引擎）、CMake  
- **打包：** Debian 包（`.deb`）

---

## 目录结构

```text
.
├── tauri-app/          # 前端（Vue + Tauri）
├── backup_project/     # 后端 C++（backup_server / 核心库）
├── scripts/            # 构建脚本（如 sidecar 编译）
├── Test/               # 测试用例与脚本
├── Resolutions/        # 环境与配置说明（可选阅读）
└── README.md
```

---

## 运行已打包程序

请优先使用已构建的 **`.deb` 安装包**（Debian / Ubuntu 及衍生发行版，`amd64`）。

### 安装

```bash
sudo apt install ./Data-Backup.deb
```

### 启动

安装后可在应用菜单中找到 **Data-Backup**。

（可执行文件名由当前打包配置决定，为 `usr/bin/tauri-app`。）

### 系统要求（简要）

- Linux **x86_64 / amd64**
- 图形桌面环境
- 常见 WebKitGTK 等运行库（由 deb 依赖声明；`apt install` 一般会自动处理）

> 说明：本仓库默认交付 **deb**。AppImage 因体积限制未作为提交产物。

---

## 从源码构建（开发）

### 依赖

- Node.js、npm  
- Rust（`rustc` / `cargo`）  
- CMake、C++ 编译工具链、OpenSSL、SQLite 等（后端构建所需）  
- Tauri / WebKitGTK 相关系统库（详见 [Resolutions/tauri/tauri_config.md](./Resolutions/tauri/tauri_config.md)）

### 开发模式

```bash
# 1. 编译 C++ sidecar 并放入 Tauri bin 目录
bash scripts/build-backup-sidecar.sh

# 2. 启动前端开发
cd tauri-app
npm install
npm run tauri dev
```