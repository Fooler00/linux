# 测试结果

当前状态：已添加 WebdriverIO/Tauri 前端 E2E 测试代码，尚未成功执行完整 E2E。

## 执行前置

运行前需要安装或提供：

- `tauri-driver`
- `WebKitWebDriver` 或 `webkit2gtk-driver`
- 空闲端口 `8080`

当前 npm 脚本会先执行 `Test/frontend/helpers/preflight.mjs`。前置条件不满足时直接退出，不启动 WDIO worker。

本机验证结果：

```text
Frontend E2E preflight failed. Missing or unavailable: tauri-driver, WebKitWebDriver/webkit2gtk-driver, free TCP port 8080
```

## 结果记录模板

| 用例 | 状态 | 输出证据 | 备注 |
|---|---|---|---|
| FE-AUTH-* | NOT_RUN | - | 等待 WebDriver 前置环境 |
| FE-BACKUP-* | NOT_RUN | - | 等待 WebDriver 前置环境 |
| FE-RESTORE-* | NOT_RUN | - | 等待 WebDriver 前置环境 |
| FE-WATCH-* | NOT_RUN | - | 等待 WebDriver 前置环境 |
| FE-SCHEDULE-* | NOT_RUN | - | 等待 WebDriver 前置环境 |
| FE-MANAGE-* | NOT_RUN | - | 等待 WebDriver 前置环境 |
| FE-CLOUD-* | NOT_RUN | - | 等待 WebDriver 前置环境 |
| FE-TASKS-* | NOT_RUN | - | 等待 WebDriver 前置环境 |

失败截图目录：`Test/output/frontend/screenshots/`

日志目录：`Test/output/frontend/logs/`
