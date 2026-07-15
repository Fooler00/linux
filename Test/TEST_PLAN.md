# 测试计划

## 目标

验证文件备份软件前端页面在真实 Tauri 应用中能够自动完成登录、备份、还原、实时监听、定时备份、备份管理、模拟云存储和任务列表操作。

## 范围

- AuthPanel.vue
- BackupPanel.vue
- RestorePanel.vue
- WatchPanel.vue
- SchedulePanel.vue
- BackupManagePanel.vue
- CloudPanel.vue
- TaskList.vue
- 页面与真实 `/api/*` 接口联动
- 异步任务状态：running、success、failed

## 不测试内容

- Vue 组件隔离单元测试。
- 原生保存文件对话框中的云存储下载 UI 流程。
- 非 8080 端口的 Tauri E2E。
- 前端 console 与 Rust backend 的官方 WDIO log forwarding。

这些限制来自当前项目未接入 `tauri-plugin-wdio`，且 Rust/Tauri 业务代码硬编码 8080。本次不修改业务代码。

## 测试层级

- 前端 E2E：WebdriverIO + `@wdio/tauri-service`。
- API 状态等待：通过 `/api/tasks`、`/api/schedules` 辅助断言异步任务。
- 文件系统验证：仅访问 `Test/output/frontend/` 内的隔离测试数据。

## 通过准则

- WDIO 用例断言通过。
- 失败用例自动保存截图。
- 测试不读取或修改用户真实数据。
- 测试结束后停止监听和定时任务。

## 失败处理

- 失败截图写入 `Test/output/frontend/screenshots/`。
- WDIO 日志写入 `Test/output/frontend/logs/`。
- 产品已知缺陷不得改写为脚本错误。

## 风险

- 当前环境缺少 `tauri-driver` 和 WebKitWebDriver，未安装前 E2E 会在 preflight 阶段失败。
- 页面没有 `data-testid`，当前选择器依赖中文文本、label 和 DOM 层级；若 UI 文案改变，需要同步更新 page object。
- Tauri E2E 受当前业务代码限制使用 8080。

