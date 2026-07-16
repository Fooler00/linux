# Backup Project Test Suite

This directory contains the automated test suite for the `v1-test` branch.

Generated data is isolated under `Test/output/`. Build output is isolated under `Test/build/`. Neither directory should be committed except `Test/output/.gitignore`.

## Build And Run

```bash
cmake -S backup_project -B Test/build/cpp -DBUILD_TESTING=ON
cmake --build Test/build/cpp --parallel
ctest --test-dir Test/build/cpp --output-on-failure
```

Run selected layers:

```bash
ctest --test-dir Test/build/cpp -L unit --output-on-failure
ctest --test-dir Test/build/cpp -L component --output-on-failure
ctest --test-dir Test/build/cpp -L integration --output-on-failure
ctest --test-dir Test/build/cpp -L api --output-on-failure
```

API tests start their own `backup_server` on an isolated port in `18080-18120`, set `BACKUP_DB` and `BACKUP_CLOUD_DIR` to `Test/output/...`, record the server PID, and clean only that PID.

Frontend E2E uses WebdriverIO + `@wdio/tauri-service`. The current environment provides `tauri-driver`, `WebKitWebDriver`, `Xvfb`, and `xvfb-run`, so stable frontend smoke checks can establish real WebDriver sessions.

Current frontend E2E classification:

- `Frontend.E2EEnv`: environment gate for `tauri-driver`, `WebKitWebDriver`, `Xvfb`, Node/npm and port readiness.
- `Frontend.E2ESmoke`: real AuthPanel interaction. It registers a user, enters username/password, clicks the registration button, and asserts the workspace is reached.
- `Frontend.E2EUISmoke`: UI navigation/page smoke. It switches tabs and verifies visible panels, key text, and controls for BackupPanel, RestorePanel, WatchPanel, SchedulePanel, BackupManagePanel, CloudPanel, and TaskList.
- `Frontend.E2EFullBusiness`: disabled in default CTest regression. Full frontend business interaction for backup, restore, management, cloud and task operations is blocked by `TEST-INFRA-001`.

The UI smoke tests do not prove complete backup, restore, cloud, management or task business flows. Those business flows are covered by the C++/CLI/API tests where stable. The frontend E2E suite checks port `8080` and `1420` before starting and will not kill unknown processes.

Run stable frontend checks:

```bash
ctest --test-dir Test/build/cpp -R 'Frontend\.E2E(Env|Smoke|UISmoke)$' --output-on-failure
```

Run non-frontend regression:

```bash
ctest --test-dir Test/build/cpp -LE frontend --output-on-failure
```

`Frontend.E2EFullBusiness` is intentionally not part of the default executable regression until WebKitWebDriver/Tauri command stability is resolved.
