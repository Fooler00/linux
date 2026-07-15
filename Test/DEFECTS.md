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

Status: `BLOCKED`

Detected by: `Frontend.E2EEnv`

Missing:

- `WebKitWebDriver` or `webkit2gtk-driver`
- `Xvfb`

Present:

- `tauri-driver` at `/home/sprtalv/.cargo/bin/tauri-driver`

Install commands:

```bash
sudo apt-get update
sudo apt-get install -y webkit2gtk-driver xvfb
```

Business code modified:

- No.
