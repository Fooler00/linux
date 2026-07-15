# Test 目录说明

本目录保存文件备份软件的测试源码、测试脚本、测试文档和运行输出规则。

## 目录结构

```text
Test/
├── README.md
├── TEST_PLAN.md
├── TEST_CASES.md
├── TEST_RESULTS.md
├── DEFECTS.md
├── TRACEABILITY_MATRIX.md
├── frontend/
│   ├── README.md
│   ├── wdio.conf.ts
│   ├── specs/
│   ├── page-objects/
│   ├── helpers/
│   │   └── preflight.mjs
│   └── manual_page_checklist.md
└── output/
    └── frontend/
```

`Test/output/` 保存截图、日志、临时数据库、临时源目录、备份目录和还原目录，默认不提交 Git。

## 环境要求

- Node.js 和 npm。
- Rust/Cargo 和已构建的 Tauri 应用。
- Tauri sidecar：`tauri-app/src-tauri/bin/backup_server-x86_64-unknown-linux-gnu`。
- Linux WebDriver 前置：`tauri-driver`、`WebKitWebDriver` 或 `webkit2gtk-driver`。
- 运行 E2E 前 8080 端口必须空闲。
- 如 WebKitWebDriver 不在 `/usr/bin/WebKitWebDriver`，可设置 `WEBKIT_WEBDRIVER_PATH`。

## 构建与运行

```bash
cd tauri-app
npm run test:e2e:tauri
```

该脚本会先执行 `Test/frontend/helpers/preflight.mjs`，前置条件缺失时不会启动 WDIO worker。

若需要先构建：

```bash
cd tauri-app
npm run test:e2e:tauri:build
```

## 输出

- 日志：`Test/output/frontend/logs/`
- 失败截图：`Test/output/frontend/screenshots/`
- 测试数据：`Test/output/frontend/source|backup|restore|cloud|downloads|tmp/`
- 运行时数据：`Test/output/frontend/runtime/`

## 清理

测试输出可以直接删除：

```bash
rm -rf Test/output/
```
