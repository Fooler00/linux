## 环境配置

后端待注明环境配置需求。  

对于前端运行环境，需要安装rust环境，系统底层web渲染相关依赖。  
详情可参考[tauri_config.md](./Resolutions/tauri/tauri_config.md)。

## 首次运行开发环境

编译获得可执行文件：

```bash
g++ -std=c++17 -DCPPHTTPLIB_OPENSSL_SUPPORT backend.cpp -o backup_server -lssl -lcrypto -lpthread -lsqlite3
```

当前sidecar命名为`backup_server`，不建议修改。

获取当前系统三元组：

```bash
rustc -vV | grep host
```

为可执行文件添加同名后缀，如`backup_server-x86_64-unknown-linux-gnu`。  
将重命名后的可执行文件移动到`tauri-app/src-tauri/bin`目录下。

在tauri-app目录下执行：

```bash
npm install
npm run tauri dev
```
