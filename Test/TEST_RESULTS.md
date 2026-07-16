# Test Results

Execution time: `2026-07-16 08:14 CST`

Branch: `v1-test`

Baseline commit: `c8a99931eb73d8467c9870b17c4e318016e280b6`

## Commands

```bash
cmake -S backup_project -B Test/build/cpp -DBUILD_TESTING=ON
cmake --build Test/build/cpp --parallel
npm ci
npm run build:web
bash Test/frontend/check_e2e_env.sh
bash Test/frontend/run_e2e_smoke.sh
bash Test/frontend/run_e2e_full.sh
ctest --test-dir Test/build/cpp -R '^API.WatchScheduleCloud$' --output-on-failure  # strict two-phase rerun repeated 10 times
ctest --test-dir Test/build/cpp -R 'Frontend\.E2E(Env|Smoke|UISmoke)$' --output-on-failure
ctest --test-dir Test/build/cpp -LE frontend --output-on-failure
npm audit
npm audit --omit=dev
ctest --test-dir Test/build/cpp -N
```

Frontend stable checks and non-frontend CTest regression were executed after E2E classification cleanup. Full frontend business-interaction E2E is disabled in default CTest because of `TEST-INFRA-001`.

## CTest Entry Summary

| Status | Count |
|---|---:|
| PASS | 67 |
| FAIL | 1 |
| SKIP | 0 |
| EXPECTED_FAIL | 0 |
| NOT_RUN | 1 |
| Total registered CTest entries | 69 |

Final executable regression is split into stable frontend smoke plus non-frontend CTest:

- Stable frontend checks: `Frontend.E2EEnv`, `Frontend.E2ESmoke`, and `Frontend.E2EUISmoke` all PASS.
- Non-frontend CTest: 65 entries, 64 PASS and 1 FAIL.
- The only failing executable CTest entry is the known `API.WatchScheduleCloud` product defect tracked as `DEF-001`.
- `Frontend.E2EFullBusiness` is disabled/NOT_RUN in default CTest.

Frontend full business-flow status, separate from CTest entry count:

| Status | Count | Flows |
|---|---:|---|
| BLOCKED/NOT_RUN | 5 | BackupPanel full backup, RestorePanel full restore, BackupManagePanel query/prune, CloudPanel upload/list/download/delete, TaskList task-state interaction |

## Layer Distribution

| Layer | Total | PASS | FAIL | BLOCKED |
|---|---:|---:|---:|---:|
| smoke | 1 | 1 | 0 | 0 |
| unit | 27 | 27 | 0 | 0 |
| component | 14 | 14 | 0 | 0 |
| integration | 18 | 18 | 0 | 0 |
| CLI | 1 | 1 | 0 | 0 |
| API/watch/schedule/cloud | 2 | 1 | 1 | 0 |
| performance | 1 | 1 | 0 | 0 |
| security | 1 | 1 | 0 | 0 |
| frontend env/Auth/UI smoke | 3 | 3 | 0 | 0 |
| frontend full business E2E | 5 | 0 | 0 | 5 |

## Failed Tests

| Test | Status | Reason |
|---|---|---|
| `API.WatchScheduleCloud` | FAIL | Strict two-phase watch-stop validation confirms a unique sentinel file created only after successful `/api/watch/stop` response is still captured by a new realtime backup. This is recorded as `DEF-001`. |
| `Frontend.E2EFullBusiness` | NOT_RUN/BLOCKED | Disabled in default CTest. Complex frontend business interactions are blocked by `TEST-INFRA-001`; they are not counted as PASS. |

Repeated defect verification on `2026-07-16 05:35-05:36 CST`:

| Run | Result | Stop response state | Final state | Sentinel evidence |
|---|---|---|---|---|
| 1 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 2 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 3 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 4 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 5 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 6 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 7 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 8 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 9 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |
| 10 | FAIL | backups/tasks 1/1 | backups/tasks 2/2 | Sentinel present in new backup. |

Conclusion: `DEF-001` is stable in the current test environment. The earlier count-only check could have been vulnerable to pre-stop queued work completing late, but the updated sentinel-based check proves the failing backup is triggered by a file created after the stop response.

Final non-frontend CTest regression on `2026-07-16 08:13 CST`:

| Metric | Count |
|---|---:|
| CTest entries | 65 |
| Passed | 64 |
| Failed | 1 |
| Skipped by CTest | 0 |

The only failing CTest entry is the known `API.WatchScheduleCloud` failure tracked as `DEF-001`.

## Frontend E2E

E2E dependency check on the current environment:

| Dependency | Result |
|---|---|
| `node` | PASS |
| `npm` | PASS |
| `cargo` | PASS |
| `tauri-driver` | PASS, found at `/home/sprtalv/.cargo/bin/tauri-driver` |
| `WebKitWebDriver` | PASS, found at `/usr/bin/WebKitWebDriver` |
| `Xvfb` | PASS, found at `/usr/bin/Xvfb` |
| `xvfb-run` | PASS, found at `/usr/bin/xvfb-run` |
| Port `8080` | PASS, free before E2E check |

E2E execution summary:

| Scope | Actual result |
|---|---|
| Environment gate | PASS |
| WebDriver session | PASS; sessions were created by `tauri-driver`/WebKitWebDriver |
| Login smoke | PASS; registered a user, entered username/password, clicked register, and reached the workspace |
| Auth smoke | PASS; real input/click/assertion completed |
| UI navigation smoke | PASS; seven page/panel smoke checks completed |
| Full business E2E | BLOCKED/NOT_RUN; disabled in default CTest due `TEST-INFRA-001` |

Frontend flow results:

| Flow | Status | Evidence |
|---|---|---|
| AuthPanel | PASS | Login/register smoke completed with input, click, and workspace assertions. |
| BackupPanel UI smoke | PASS | Tab switched, panel visible, backup form labels and `开始备份` control displayed. Full backup operation through UI is BLOCKED/NOT_RUN. |
| RestorePanel UI smoke | PASS | Tab switched, panel visible, restore form labels and `开始还原` control displayed. Full restore operation through UI is BLOCKED/NOT_RUN. |
| WatchPanel UI smoke | PASS | `启动监听` and `停止监听` controls displayed. Full watch business behavior is covered by API tests; current product defect is `DEF-001`. |
| SchedulePanel UI smoke | PASS | `启动定时备份` and `运行中的定时任务` displayed. Full schedule behavior is covered by API tests. |
| BackupManagePanel UI smoke | PASS | Management panel, query/prune controls, target path label, and initial guidance displayed. Full query/prune operation through UI is BLOCKED/NOT_RUN. |
| CloudPanel UI smoke | PASS | Current Token label, `保存`, `刷新列表`, and `上传到云端` controls displayed. Full cloud operation through UI is BLOCKED/NOT_RUN. |
| TaskList UI smoke | PASS | `手动刷新` and `暂无任务` displayed. Full task-state interaction through UI is BLOCKED/NOT_RUN. |

The old frontend assertions for `请检查备份表单`, `请检查还原表单`, `暂无备份记录`, `保存 Token`, and `刷新` were test-code expectations from an earlier UI shape, not confirmed product defects. They were corrected to verify current visible UI smoke behavior.

Stable frontend CTest run on `2026-07-16 08:00-08:12 CST`:

| Test | Result | Duration |
|---|---|---:|
| `Frontend.E2EEnv` | PASS | 0.22s |
| `Frontend.E2ESmoke` | PASS | 116.41s |
| `Frontend.E2EUISmoke` | PASS | 607.70s |

`Frontend.E2EUISmoke` is slow because of WebKitWebDriver/Tauri command latency. It is retained as page smoke only, not as a full business-flow pass.

Logs and screenshots:

- WDIO logs: `Test/output/frontend/wdio-logs/`
- Vite logs: `Test/output/frontend/vite-smoke.log`, `Test/output/frontend/vite-full.log`
- Failure screenshots: `Test/output/frontend/screenshots/`

## Notes

- GoogleTest source: FetchContent `v1.17.0`, commit `52eb8108c5bdec04579160ae17225d66034bd723`.
- `npm ci` completed successfully.
- `@wdio/tauri-service@1.2.0` required `@wdio/native-utils` exports that are only present in `2.5.0`; `tauri-app/package.json` now uses an npm override to lock that nested test dependency.
- `npm run build:web` completed successfully.
- `npm audit` reports 3 vulnerabilities in the dev dependency tree through `@wdio/mocha-framework -> mocha -> serialize-javascript`; no automatic upgrade was performed because the suggested fix is breaking.
- `npm audit --omit=dev` reports 0 production vulnerabilities.
- Test output and generated databases/logs/screenshots are under `Test/output/`.
