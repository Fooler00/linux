# Test Results

Execution time: `2026-07-16 05:40 CST`

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
npm audit
npm audit --omit=dev
ctest --test-dir Test/build/cpp -N
ctest --test-dir Test/build/cpp --output-on-failure
```

The full CTest run was executed outside the sandbox because API tests require local loopback port access.

## Summary

| Status | Count |
|---|---:|
| PASS | 64 |
| FAIL | 1 |
| SKIP | 0 |
| EXPECTED_FAIL | 0 |
| NOT_RUN | 0 |
| BLOCKED | 3 |
| Total | 68 |

CTest reported 3 skipped tests. They are counted as `BLOCKED` here because the cause is missing system E2E dependencies.

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
| frontend E2E | 3 | 0 | 0 | 3 |

## Failed Tests

| Test | Status | Reason |
|---|---|---|
| `API.WatchScheduleCloud` | FAIL | Strict two-phase watch-stop validation confirms a unique sentinel file created only after successful `/api/watch/stop` response is still captured by a new realtime backup. This is recorded as `DEF-001`. |

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

Final full CTest regression on `2026-07-16 05:40 CST`:

| Metric | Count |
|---|---:|
| CTest entries | 68 |
| Passed | 64 |
| Failed | 1 |
| Skipped by CTest | 3 |

The only failing CTest entry is the known `API.WatchScheduleCloud` failure tracked as `DEF-001`. The three skipped entries are the known frontend E2E environment gates. No additional CTest failures were introduced.

## Blocked Tests

| Test | Status | Missing prerequisite |
|---|---|---|
| `Frontend.E2EEnv` | BLOCKED | `WebKitWebDriver/webkit2gtk-driver`, `Xvfb` |
| `Frontend.E2ESmoke` | BLOCKED | Same E2E system dependencies |
| `Frontend.E2EFull` | BLOCKED | Same E2E system dependencies |

E2E dependency check:

| Dependency | Result |
|---|---|
| `node` | PASS |
| `npm` | PASS |
| `cargo` | PASS |
| `tauri-driver` | PASS, found at `/home/sprtalv/.cargo/bin/tauri-driver`; this binary does not support `--version` and reports usage through `--help` |
| `WebKitWebDriver` / `webkit2gtk-driver` | BLOCKED, command not found |
| `Xvfb` | BLOCKED, command not found |
| Port `8080` | PASS, free before E2E check |

Install commands, not executed automatically:

```bash
sudo apt-get update
sudo apt-get install -y webkit2gtk-driver xvfb
```

## Notes

- GoogleTest source: FetchContent `v1.17.0`, commit `52eb8108c5bdec04579160ae17225d66034bd723`.
- `npm ci` completed successfully.
- `npm run build:web` completed successfully.
- `npm audit` reports 3 vulnerabilities in the dev dependency tree through `@wdio/mocha-framework -> mocha -> serialize-javascript`; no automatic upgrade was performed because the suggested fix is breaking.
- `npm audit --omit=dev` reports 0 production vulnerabilities.
- Test output and generated databases/logs/screenshots are under `Test/output/`.
