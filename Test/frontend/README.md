# Frontend E2E

This directory contains WebdriverIO + Tauri E2E tests for the Vue/Tauri frontend.

The current suite uses only basic WebDriver operations: element lookup, input, click, wait, DOM assertions, and failure screenshots. It does not use Tauri command mocking, `browser.tauri.execute`, or Tauri/Rust business-code changes.

Run environment check:

```bash
bash Test/frontend/check_e2e_env.sh
```

Run smoke:

```bash
cd tauri-app
npm run test:e2e:tauri:smoke
```

Run full E2E:

```bash
cd tauri-app
npm run test:e2e:tauri
```
