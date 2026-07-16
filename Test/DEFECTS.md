# Defects

## DEF-001: Realtime watch may create one extra backup after stop

Status: `FAIL`

Reverification: stable, 10 failures out of 10 strict two-phase reruns on `2026-07-16`.

Detected by: `API.WatchScheduleCloud`

Reproduction:

1. Start `backup_server` on an isolated port.
2. Call `POST /api/watch/start` with `intervalSeconds=1`.
3. Create `phase_a_trigger.txt` and confirm it creates a realtime task and backup before stop.
4. Wait until the backup count is stable before calling stop.
5. Call `POST /api/watch/stop` and confirm the response is `{"message":"实时备份已停止"}`.
6. Record backup count, task count, and stop response time.
7. Create a unique `sentinel_after_stop_*.txt` only after the stop response returns.
8. Wait 3 seconds, longer than the configured 1 second watch interval.
9. Check whether any new backup contains the sentinel file.

Expected:

- No new backup should be created after `/api/watch/stop` returns.
- A file created only after a successful stop response should not appear in any realtime backup.

Actual:

- A new realtime task and backup are created after stop.
- The new backup contains the sentinel file that was created after the stop response returned.

Synchronization assessment:

- The earlier count-only assertion could have misclassified a backup that was triggered before stop but completed after stop.
- The current strict two-phase test removes that ambiguity by confirming the pre-stop trigger has completed, waiting for the backup count to stabilize, then creating a unique sentinel only after the stop response.
- Because the sentinel file appears in a new backup in every rerun, the current evidence points to incomplete product-side watch stop behavior rather than a test synchronization issue.

Repeated verification:

| Run | Result | Evidence |
|---|---|---|
| 1 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053525`. |
| 2 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053535`. |
| 3 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053545`. |
| 4 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053555`. |
| 5 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053605`. |
| 6 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053615`. |
| 7 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053624`. |
| 8 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053634`. |
| 9 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053644`. |
| 10 | FAIL | Stop response returned with backup/task count 1/1; sentinel created after stop; final backup/task count 2/2; sentinel present in `backup_20260716_053654`. |

Likely cause:

- The detached watch thread checks `impl_->watching` at the top of the loop, sleeps, and then proceeds to snapshot/backup without checking `impl_->watching` again immediately after sleep.

Business code modified:

- No.

Minimum suggested product fix:

- In the watch thread, re-check `impl_->watching` immediately after the sleep and before snapshot comparison or backup creation.

## E2E-BLOCKED-001: Frontend Tauri E2E system dependencies missing

Status: resolved; environment gate now `PASS`

Detected by: `Frontend.E2EEnv`

Previously missing:

- `WebKitWebDriver` or `webkit2gtk-driver`
- `Xvfb`

Present now:

- `tauri-driver` at `/home/sprtalv/.cargo/bin/tauri-driver`
- `WebKitWebDriver` at `/usr/bin/WebKitWebDriver`
- `Xvfb` at `/usr/bin/Xvfb`
- `xvfb-run` at `/usr/bin/xvfb-run`

Business code modified:

- No.

## TEST-CODE-001: Frontend E2E assertions used obsolete visible text

Status: fixed in test code

Detected by: standalone frontend E2E calibration

Old assertions:

| Flow | Obsolete assertion | Current handling |
|---|---|---|
| BackupPanel | `请检查备份表单` | Replaced with UI smoke for current backup form labels and `开始备份`. |
| RestorePanel | `请检查还原表单` | Replaced with UI smoke for current restore form labels and `开始还原`. |
| BackupManagePanel | `暂无备份记录` before querying | Replaced with UI smoke for current query/prune controls and initial guidance text. |
| CloudPanel | `保存 Token` | Replaced with current `保存` button and Token label. |
| TaskList | `刷新` | Replaced with current `手动刷新` and `暂无任务`. |

Conclusion:

- These five failures are not currently classified as product defects.
- They were stale E2E assertions or incorrect test preconditions against the current UI.
- No Vue, Rust, C++ or business code was modified.

## TEST-INFRA-001: WebKitWebDriver/Tauri commands can hang during complex frontend interaction

Status: active test infrastructure limitation

Detected by: frontend E2E calibration

Evidence:

- WebDriver sessions can be created and the AuthPanel smoke test passes with real input, click, and workspace assertions.
- UI navigation/page smoke can pass, but it is slow; the latest stable UI smoke took about 607.70 seconds.
- During attempts to exercise more complex panel interactions, WebKitWebDriver/Tauri sessions hung or timed out on commands such as `executeAsyncScript`, visibility checks, click chains, and panel status queries.

Impact:

- Stable default frontend coverage is limited to `Frontend.E2EEnv`, `Frontend.E2ESmoke`, and `Frontend.E2EUISmoke`.
- Full frontend business flows for BackupPanel, RestorePanel, BackupManagePanel, CloudPanel, and TaskList are recorded as BLOCKED/NOT_RUN.
- This is not attributed to Vue product behavior without stronger evidence.
- No business code change is required by this test infrastructure finding.

Current mitigation:

- `Frontend.E2EFullBusiness` is disabled in default CTest regression.
- Business behavior remains covered through C++/CLI/API tests where stable.
- Logs and screenshots remain under `Test/output/frontend/`.
