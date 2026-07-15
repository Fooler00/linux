#!/usr/bin/env bash

set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/scripts/common.sh"

start_test_server || exit 1

api="http://127.0.0.1:$BACKUP_PORT"

register_body='{"username":"api_user","password":"api_pass"}'
register_resp="$(curl -fsS -H 'Content-Type: application/json' -d "$register_body" "$api/api/register" 2>"$RUN_DIR/register.err")"
token="$(printf '%s' "$register_resp" | json_value token)"
[[ -n "$token" ]] && pass "API register returns token" || fail "API register returns token"

login_resp="$(curl -fsS -H 'Content-Type: application/json' -d "$register_body" "$api/api/login")"
assert_contains "$login_resp" "api_user" "API login returns username"

wrong_status="$(curl -sS -o "$RUN_DIR/wrong_login.json" -w '%{http_code}' -H 'Content-Type: application/json' -d '{"username":"api_user","password":"wrong"}' "$api/api/login")"
[[ "$wrong_status" == "401" ]] && pass "API wrong password returns 401" || fail "API wrong password returns 401 ($wrong_status)"

src="$RUN_DIR/source"
dst="$RUN_DIR/backups"
restore="$RUN_DIR/restore"
mkdir -p "$src/nested" "$dst" "$restore"
printf 'api-alpha' >"$src/a.txt"
printf 'api-beta' >"$src/nested/b.txt"

backup_payload="$(printf '{"source":"%s","destination":"%s","compress":false,"filter":{"archiveType":"none"}}' "$src" "$dst")"
backup_resp="$(curl -fsS -H 'Content-Type: application/json' -H "Authorization: Bearer $token" -d "$backup_payload" "$api/api/backup")"
task_id="$(printf '%s' "$backup_resp" | json_number taskId)"
[[ -n "$task_id" ]] && pass "API backup returns taskId" || fail "API backup returns taskId"
if wait_task_status "$task_id" success; then pass "API backup task reaches success"; else fail "API backup task reaches success"; fi

backups_resp="$(curl -fsS "$api/api/backups?destination=$(printf '%s' "$dst" | sed 's/ /%20/g')")"
assert_contains "$backups_resp" "backup_" "API backup list returns entry"
backup_path="$(find "$dst" -maxdepth 1 -type d -name 'backup_*' | head -1)"

meta_status="$(curl -sS -o "$RUN_DIR/metadata.json" -w '%{http_code}' "$api/api/metadata?path=$(printf '%s' "$backup_path" | sed 's/ /%20/g')")"
[[ "$meta_status" == "200" ]] && pass "API metadata returns 200" || fail "API metadata returns 200 ($meta_status)"
assert_contains "$(cat "$RUN_DIR/metadata.json")" "copiedFiles" "API metadata contains copiedFiles"

restore_payload="$(printf '{"backupPath":"%s","destination":"%s"}' "$backup_path" "$restore")"
restore_resp="$(curl -fsS -H 'Content-Type: application/json' -d "$restore_payload" "$api/api/restore")"
restore_task="$(printf '%s' "$restore_resp" | json_number taskId)"
if wait_task_status "$restore_task" success; then pass "API restore task reaches success"; else fail "API restore task reaches success"; fi
restored_root="$(find "$restore" -maxdepth 1 -type d -name 'backup_*' | head -1)"
assert_equal_file "$src/a.txt" "$restored_root/a.txt" "API restore preserves content"

tasks_resp="$(curl -fsS "$api/api/tasks")"
assert_contains "$tasks_resp" '"status":"success"' "API tasks includes successful task"

prune_resp="$(curl -fsS -H 'Content-Type: application/json' -d "$(printf '{"destination":"%s","maxBackups":5,"maxAgeDays":0}' "$dst")" "$api/api/prune")"
assert_contains "$prune_resp" "removed" "API prune returns removed count"

bad_status="$(curl -sS -o "$RUN_DIR/bad_backup.json" -w '%{http_code}' -H 'Content-Type: application/json' -d '{"destination":""}' "$api/api/backup")"
[[ "$bad_status" == "400" ]] && pass "API missing source/destination returns 400" || fail "API missing source/destination returns 400 ($bad_status)"

tasks_after_fail="$(curl -fsS "$api/api/tasks")"
assert_contains "$tasks_after_fail" "[" "API remains responsive after failed request"
