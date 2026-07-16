#!/usr/bin/env bash

set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/scripts/common.sh"

start_test_server || exit 1
api="http://127.0.0.1:$BACKUP_PORT"

src="$RUN_DIR/watch-source"
dst="$RUN_DIR/watch-backups"
mkdir -p "$src" "$dst"
printf 'initial' >"$src/file.txt"

timeline="$RUN_DIR/watch_timeline.log"
touch "$timeline"

ts_ms() {
  printf '%s\n' "$(( $(date +%s%N) / 1000000 ))"
}

watch_log() {
  printf '%s %s\n' "$(date '+%Y-%m-%d %H:%M:%S.%3N')" "$*" | tee -a "$timeline"
}

backup_count() {
  find "$dst" -maxdepth 1 -name 'backup_*' | wc -l | tr -d ' '
}

tasks_json() {
  curl -fsS "$api/api/tasks" 2>/dev/null || printf '[]'
}

task_count() {
  tasks_json | grep -o '"id":[0-9]*' | wc -l | tr -d ' '
}

realtime_task_count() {
  tasks_json | grep -o '"type":"realtime-backup"' | wc -l | tr -d ' '
}

task_ids_and_status() {
  tasks_json | grep -o '"id":[0-9]*\|"type":"[^"]*"\|"status":"[^"]*"' | paste - - - 2>/dev/null || true
}

wait_for_backup_count_at_least() {
  local expected="$1"
  local timeout_seconds="$2"
  local start
  start="$(ts_ms)"
  while (( $(ts_ms) - start < timeout_seconds * 1000 )); do
    if (( $(backup_count) >= expected )); then
      watch_log "wait_for_backup_count_at_least expected=$expected actual=$(backup_count) elapsed_ms=$(( $(ts_ms) - start ))"
      return 0
    fi
    sleep 0.2
  done
  watch_log "wait_for_backup_count_at_least timeout expected=$expected actual=$(backup_count) elapsed_ms=$(( $(ts_ms) - start ))"
  return 1
}

wait_for_realtime_task_count_at_least() {
  local expected="$1"
  local timeout_seconds="$2"
  local start
  start="$(ts_ms)"
  while (( $(ts_ms) - start < timeout_seconds * 1000 )); do
    if (( $(realtime_task_count) >= expected )); then
      watch_log "wait_for_realtime_task_count_at_least expected=$expected actual=$(realtime_task_count) elapsed_ms=$(( $(ts_ms) - start ))"
      return 0
    fi
    sleep 0.2
  done
  watch_log "wait_for_realtime_task_count_at_least timeout expected=$expected actual=$(realtime_task_count) elapsed_ms=$(( $(ts_ms) - start ))"
  return 1
}

wait_for_backup_count_stable() {
  local stable_window_seconds="$1"
  local timeout_seconds="$2"
  local start stable_start last_count current_count
  start="$(ts_ms)"
  stable_start="$start"
  last_count="$(backup_count)"
  while (( $(ts_ms) - start < timeout_seconds * 1000 )); do
    sleep 0.2
    current_count="$(backup_count)"
    if [[ "$current_count" == "$last_count" ]]; then
      if (( $(ts_ms) - stable_start >= stable_window_seconds * 1000 )); then
        watch_log "backup_count_stable count=$current_count stable_ms=$(( $(ts_ms) - stable_start )) elapsed_ms=$(( $(ts_ms) - start ))"
        return 0
      fi
    else
      watch_log "backup_count_changed previous=$last_count current=$current_count elapsed_ms=$(( $(ts_ms) - start ))"
      last_count="$current_count"
      stable_start="$(ts_ms)"
    fi
  done
  watch_log "backup_count_stable timeout count=$(backup_count) elapsed_ms=$(( $(ts_ms) - start ))"
  return 1
}

find_backups_containing_file() {
  local name="$1"
  find "$dst" -maxdepth 2 -path "*/$name" -print | sed "s#/$name##" | sort -u
}

watch_payload="$(printf '{"source":"%s","destination":"%s","intervalSeconds":1,"filter":{"archiveType":"none"}}' "$src" "$dst")"
watch_log "watch/start request backup_count=$(backup_count) task_count=$(task_count)"
watch_resp="$(curl -fsS -H 'Content-Type: application/json' -d "$watch_payload" "$api/api/watch/start")"
assert_contains "$watch_resp" "实时备份已启动" "API watch start succeeds"
watch_log "watch/start response backup_count=$(backup_count) task_count=$(task_count) response=$watch_resp"
sleep 0.5
watch_log "watch/start initial_snapshot_window elapsed_ms=500 backup_count=$(backup_count) task_count=$(task_count)"

duplicate_status="$(curl -sS -o "$RUN_DIR/watch_duplicate.json" -w '%{http_code}' -H 'Content-Type: application/json' -d "$watch_payload" "$api/api/watch/start")"
[[ "$duplicate_status" == "400" ]] && pass "API duplicate watch start returns 400" || fail "API duplicate watch start returns 400 ($duplicate_status)"

phase_a_task_count="$(realtime_task_count)"
phase_a_backup_count="$(backup_count)"
phase_a_file="phase_a_trigger.txt"
watch_log "phaseA before_modify file=$phase_a_file backup_count=$phase_a_backup_count realtime_task_count=$phase_a_task_count"
printf 'phase-a-change' >"$src/$phase_a_file"
watch_log "phaseA file_created file=$phase_a_file created_ms=$(ts_ms)"
if wait_for_realtime_task_count_at_least "$((phase_a_task_count + 1))" 8 &&
   wait_for_backup_count_at_least "$((phase_a_backup_count + 1))" 8; then
  pass "API watch creates realtime task and backup before stop"
else
  fail "API watch creates realtime task and backup before stop"
fi
if [[ -n "$(find_backups_containing_file "$phase_a_file")" ]]; then
  pass "API watch phase A backup contains trigger file"
else
  fail "API watch phase A backup contains trigger file"
fi
if wait_for_backup_count_stable 2 8; then
  pass "API watch backup count stabilizes before stop"
else
  fail "API watch backup count stabilizes before stop"
fi
watch_log "phaseA stable backup_count=$(backup_count) task_count=$(task_count) realtime_task_count=$(realtime_task_count)"

stop_request_ms="$(ts_ms)"
pre_stop_count="$(backup_count)"
pre_stop_tasks="$(task_count)"
watch_log "watch/stop request request_ms=$stop_request_ms pre_stop_backup_count=$pre_stop_count pre_stop_task_count=$pre_stop_tasks"
stop_resp="$(curl -fsS -H 'Content-Type: application/json' -d '{}' "$api/api/watch/stop")"
stop_response_ms="$(ts_ms)"
assert_contains "$stop_resp" "实时备份已停止" "API watch stop succeeds"
post_stop_count="$(backup_count)"
post_stop_tasks="$(task_count)"
watch_log "watch/stop response response_ms=$stop_response_ms post_stop_backup_count=$post_stop_count post_stop_task_count=$post_stop_tasks response=$stop_resp"

sentinel_file="sentinel_after_stop_$(date +%s%N).txt"
sentinel_created_ms="$(ts_ms)"
printf 'sentinel-after-stop' >"$src/$sentinel_file"
watch_log "phaseB sentinel_created file=$sentinel_file created_ms=$sentinel_created_ms backup_count=$(backup_count) task_count=$(task_count)"

sleep_start_ms="$(ts_ms)"
sleep 3
sleep_end_ms="$(ts_ms)"
final_count="$(backup_count)"
final_tasks="$(task_count)"
sentinel_backups="$(find_backups_containing_file "$sentinel_file")"
watch_log "phaseB final sleep_ms=$((sleep_end_ms - sleep_start_ms)) final_backup_count=$final_count final_task_count=$final_tasks sentinel_backups=${sentinel_backups:-none}"
watch_log "phaseB tasks $(task_ids_and_status | tr '\n' ';')"
if [[ -z "$sentinel_backups" ]]; then
  pass "API watch stop prevents sentinel file backup after stop"
else
  fail "API watch stop prevents sentinel file backup after stop (sentinel_backups=$sentinel_backups)"
fi

bad_watch_status="$(curl -sS -o "$RUN_DIR/watch_bad.json" -w '%{http_code}' -H 'Content-Type: application/json' -d '{"source":"/definitely/missing","destination":"/tmp","intervalSeconds":0}' "$api/api/watch/start")"
[[ "$bad_watch_status" == "400" ]] && pass "API watch invalid interval returns 400" || fail "API watch invalid interval returns 400 ($bad_watch_status)"

schedule_src="$RUN_DIR/schedule-source"
schedule_dst="$RUN_DIR/schedule-backups"
mkdir -p "$schedule_src" "$schedule_dst"
printf 'scheduled' >"$schedule_src/file.txt"
schedule_payload="$(printf '{"source":"%s","destination":"%s","intervalSeconds":1,"maxBackups":2,"filter":{"archiveType":"none"}}' "$schedule_src" "$schedule_dst")"
schedule_resp="$(curl -fsS -H 'Content-Type: application/json' -d "$schedule_payload" "$api/api/schedule/start")"
schedule_id="$(printf '%s' "$schedule_resp" | json_number scheduleId)"
[[ -n "$schedule_id" ]] && pass "API schedule start returns scheduleId" || fail "API schedule start returns scheduleId"
sleep 3
schedule_count="$(find "$schedule_dst" -maxdepth 1 -name 'backup_*' | wc -l)"
(( schedule_count >= 1 )) && pass "API schedule creates backups" || fail "API schedule creates backups"
(( schedule_count <= 2 )) && pass "API schedule respects maxBackups" || fail "API schedule respects maxBackups"
schedules_resp="$(curl -fsS "$api/api/schedules")"
assert_contains "$schedules_resp" "$schedule_id" "API schedules lists active schedule"
stop_schedule_resp="$(curl -fsS -H 'Content-Type: application/json' -d "$(printf '{"scheduleId":%s}' "$schedule_id")" "$api/api/schedule/stop")"
assert_contains "$stop_schedule_resp" "true" "API schedule stop succeeds"
bad_schedule_status="$(curl -sS -o "$RUN_DIR/schedule_bad.json" -w '%{http_code}' -H 'Content-Type: application/json' -d '{"source":"/tmp","destination":"/tmp","intervalSeconds":0}' "$api/api/schedule/start")"
[[ "$bad_schedule_status" == "400" ]] && pass "API schedule invalid interval returns 400" || fail "API schedule invalid interval returns 400 ($bad_schedule_status)"

cloud_local="$RUN_DIR/cloud local.txt"
cloud_download="$RUN_DIR/cloud downloaded.txt"
printf 'cloud-data' >"$cloud_local"
upload_resp="$(curl -fsS -F "remotePath=中文 目录/remote file.txt" -F "file=@$cloud_local" "$api/api/cloud/upload")"
assert_contains "$upload_resp" "上传成功" "API cloud upload succeeds"
list_resp="$(curl -fsS "$api/api/cloud/list?dir=$(printf '%s' '中文 目录' | sed 's/ /%20/g')")"
assert_contains "$list_resp" "remote file.txt" "API cloud list returns uploaded file"
curl -fsS "$api/api/cloud/download?remotePath=$(printf '%s' '中文 目录/remote file.txt' | sed 's/ /%20/g')" -o "$cloud_download"
assert_equal_file "$cloud_local" "$cloud_download" "API cloud download preserves content"
delete_resp="$(curl -fsS -H 'Content-Type: application/json' -d '{"remotePath":"中文 目录/remote file.txt"}' "$api/api/cloud/delete")"
assert_contains "$delete_resp" "删除成功" "API cloud delete succeeds"
missing_status="$(curl -sS -o "$RUN_DIR/cloud_missing.bin" -w '%{http_code}' "$api/api/cloud/download?remotePath=missing.txt")"
[[ "$missing_status" == "500" ]] && pass "API cloud missing download returns failure" || fail "API cloud missing download returns failure ($missing_status)"
traversal_status="$(curl -sS -o "$RUN_DIR/cloud_traversal.json" -w '%{http_code}' -H 'Content-Type: application/json' -d '{"remotePath":"../escape.txt"}' "$api/api/cloud/delete")"
[[ "$traversal_status" == "500" || "$traversal_status" == "400" ]] && pass "API cloud traversal delete fails" || fail "API cloud traversal delete fails ($traversal_status)"
