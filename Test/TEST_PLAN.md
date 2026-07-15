# Test Plan

Branch: `v1-test`

Baseline: `c8a99931eb73d8467c9870b17c4e318016e280b6`

## Scope

The suite covers:

- GoogleTest + CTest infrastructure smoke
- C++ unit tests for utility, filtering, manifest, auth and task public interfaces
- C++ component tests for archive, crypto, SQLite user auth, and local cloud storage
- C++ integration tests for backup, restore, metadata, manifest, incremental behavior, symlink and historical defect reproduction
- Shell flow tests for CLI, API, realtime watch, schedule, cloud, performance and security
- WebdriverIO + Tauri E2E configuration and specs for 8 frontend flows
- Markdown documentation, defects and traceability

Out of scope:

- Modifying product source code to make tests pass
- Changing Vue/Rust business behavior
- Using real user data, real `users.db`, or project cloud storage data
- Running sudo package installation

## Test Environment

- CMake/CTest: 4.2.3
- Compiler: `g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0`
- Node: `v22.22.1`
- npm: `9.2.0`
- GoogleTest: FetchContent `v1.17.0`
- Test output: `Test/output/`
- Test build: `Test/build/cpp`

## Execution Strategy

1. Configure and build C++ tests with `BUILD_TESTING=ON`.
2. Run all CTest tests.
3. For API/Shell tests, use isolated ports and temporary DB/cloud directories under `Test/output/`.
4. For frontend E2E, first run system dependency and port checks. If `tauri-driver`, WebKit WebDriver or Xvfb are missing, mark E2E as `BLOCKED`.
5. Preserve product failures as failed tests and document them in `DEFECTS.md`.

## Status Values

- `PASS`: executed and met expected result
- `FAIL`: executed and did not meet expected result
- `SKIP`: intentionally skipped because optional runtime/tool is unavailable
- `EXPECTED_FAIL`: stable product defect accepted for this run
- `NOT_RUN`: not executed
- `BLOCKED`: cannot execute until environment or external prerequisite is satisfied
