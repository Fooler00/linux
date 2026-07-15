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

Frontend E2E uses WebdriverIO + `@wdio/tauri-service`. `tauri-driver` is available at `/home/sprtalv/.cargo/bin/tauri-driver`, but the current environment still lacks WebKit WebDriver and Xvfb, so E2E remains blocked before a WebDriver session can start.

```bash
sudo apt-get update
sudo apt-get install -y webkit2gtk-driver xvfb
```

The E2E suite checks port `8080` before starting and will not kill unknown processes.
