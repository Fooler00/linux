# 前端 Tauri E2E 测试

本目录使用 WebdriverIO + `@wdio/tauri-service` 驱动真实 Tauri 应用。

## 运行

```bash
cd tauri-app
npm run test:e2e:tauri
```

脚本会先运行 `Test/frontend/helpers/preflight.mjs`。前置条件不满足时直接退出，不启动 WDIO worker。

先构建再运行：

```bash
cd tauri-app
npm run test:e2e:tauri:build
```

## 前置条件

- `tauri-app/src-tauri/target/debug/tauri-app` 存在。
- `tauri-app/src-tauri/bin/backup_server-x86_64-unknown-linux-gnu` 存在。
- `tauri-driver` 在 PATH 中。
- `WebKitWebDriver` 或 `webkit2gtk-driver` 在 PATH 中。
- 如 WebKitWebDriver 不在 `/usr/bin/WebKitWebDriver`，设置 `WEBKIT_WEBDRIVER_PATH` 指向实际路径。
- 8080 端口空闲。

## 输出

- 失败截图：`Test/output/frontend/screenshots/`
- WDIO 日志：`Test/output/frontend/logs/`
- 临时源/备份/还原/云存储数据：`Test/output/frontend/`

## 选择器策略

不修改页面代码，不新增 `data-testid`。测试使用可见中文文本、label、placeholder、section 标题和局部 XPath 定位。
