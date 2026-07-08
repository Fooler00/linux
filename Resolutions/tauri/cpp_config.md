参考环境：linux opensuse

## 配置C++核心作为Sidecar

tauri要求嵌入的二进制文件必须携带有当前系统的目标三元组（Target Triple）后缀。  

查看三元组：

```bash
rustc -vV | grep host
```

示例输出(opensuse tumbleweed)：

```bash
host: x86_64-unknown-linux-gnu
```

将编译获得的backup可执行文件重命名，添加后缀`-x86_64-unknown-linux-gnu`。

将该文件放入tauri项目`src-tauri/bin`目录下。

在配置文件`tauri.conf.json`中注册C++程序。在bundle配置项中添加`externalBin`字段，并添加C++程序路径。

>注意：该路径中不要写三元组后缀，tauri将自动补全。

## 安装shell插件（用于调用C++）

在rust后端目录执行cargo命令：

```bash
cd src-tauri
cargo add tauri-plugin-shell
```

在rust的入口文件`src-tauri/src/lib.rs`中注册该插件:

```rust
tauri::Builder::default()
    // 注册shell插件
    .plugin(tauri_plugin_shell::init())
```

## 在前端开发C++运行权限

`src-tauri/capabilities/default.json`中显式允许调用C++程序：