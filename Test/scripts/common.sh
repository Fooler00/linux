#!/usr/bin/env bash

set -u

TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$TEST_ROOT/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$TEST_ROOT/build/cpp}"
OUTPUT_ROOT="${OUTPUT_ROOT:-$TEST_ROOT/output}"
RUN_ID="${RUN_ID:-$(date +%Y%m%d_%H%M%S)_$$}"
RUN_DIR="${RUN_DIR:-$OUTPUT_ROOT/$RUN_ID}"
SERVER_PID=""

mkdir -p "$RUN_DIR"

pass_count=0
fail_count=0

log() {
  printf '[%s] %s\n' "$(date +%H:%M:%S)" "$*"
}

pass() {
  pass_count=$((pass_count + 1))
  printf 'PASS %s\n' "$*"
}

fail() {
  fail_count=$((fail_count + 1))
  printf 'FAIL %s\n' "$*" >&2
}

skip() {
  printf 'SKIP %s\n' "$*" >&2
  exit 77
}

finish() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    if kill -0 "$SERVER_PID" 2>/dev/null; then
      kill "$SERVER_PID" 2>/dev/null || true
      wait "$SERVER_PID" 2>/dev/null || true
    fi
  fi
  if (( fail_count > 0 )); then
    printf 'SUMMARY PASS=%d FAIL=%d\n' "$pass_count" "$fail_count" >&2
    exit 1
  fi
  printf 'SUMMARY PASS=%d FAIL=0\n' "$pass_count"
}

trap finish EXIT

require_command() {
  command -v "$1" >/dev/null 2>&1 || skip "missing command: $1"
}

assert_file_exists() {
  [[ -e "$1" ]] && pass "$2" || fail "$2 ($1 missing)"
}

assert_file_not_exists() {
  [[ ! -e "$1" ]] && pass "$2" || fail "$2 ($1 exists)"
}

assert_contains() {
  local haystack="$1"
  local needle="$2"
  local name="$3"
  [[ "$haystack" == *"$needle"* ]] && pass "$name" || fail "$name (missing $needle)"
}

assert_equal_file() {
  cmp -s "$1" "$2" && pass "$3" || fail "$3 ($1 != $2)"
}

port_open() {
  local port="$1"
  timeout 1 bash -c ":</dev/tcp/127.0.0.1/$port" >/dev/null 2>&1
}

choose_port() {
  local port
  for port in $(seq 18080 18120); do
    if ! port_open "$port"; then
      printf '%s\n' "$port"
      return 0
    fi
  done
  return 1
}

wait_http() {
  local url="$1"
  local deadline=$((SECONDS + 20))
  while (( SECONDS < deadline )); do
    if curl -fsS "$url" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.2
  done
  return 1
}

start_test_server() {
  require_command curl
  local port
  port="$(choose_port)" || skip "no free test port in 18080-18120"
  export BACKUP_PORT="$port"
  export BACKUP_DB="$RUN_DIR/users.db"
  export BACKUP_CLOUD_DIR="$RUN_DIR/cloud_storage"
  mkdir -p "$BACKUP_CLOUD_DIR"
  local server="$BUILD_DIR/backup_server"
  [[ -x "$server" ]] || skip "backup_server not built at $server"
  BACKUP_PORT="$BACKUP_PORT" BACKUP_DB="$BACKUP_DB" BACKUP_CLOUD_DIR="$BACKUP_CLOUD_DIR" \
    "$server" >"$RUN_DIR/backup_server.log" 2>&1 &
  SERVER_PID=$!
  printf '%s\n' "$SERVER_PID" >"$RUN_DIR/backup_server.pid"
  if ! wait_http "http://127.0.0.1:$BACKUP_PORT/api/tasks"; then
    fail "backup_server started and became reachable"
    return 1
  fi
  pass "backup_server started on isolated port $BACKUP_PORT"
}

json_value() {
  local key="$1"
  sed -n "s/.*\"$key\"[[:space:]]*:[[:space:]]*\"\\([^\"]*\\)\".*/\\1/p"
}

json_number() {
  local key="$1"
  sed -n "s/.*\"$key\"[[:space:]]*:[[:space:]]*\\([0-9][0-9]*\\).*/\\1/p"
}

wait_task_status() {
  local task_id="$1"
  local expected="$2"
  local deadline=$((SECONDS + 25))
  while (( SECONDS < deadline )); do
    local tasks
    tasks="$(curl -fsS "http://127.0.0.1:$BACKUP_PORT/api/tasks" 2>/dev/null || true)"
    if [[ "$tasks" == *"\"id\":$task_id"* && "$tasks" == *"\"status\":\"$expected\""* ]]; then
      return 0
    fi
    if [[ "$tasks" == *"\"id\":$task_id"* && "$tasks" == *"\"status\":\"failed\""* ]]; then
      printf '%s\n' "$tasks" >"$RUN_DIR/task_${task_id}_failed.json"
      return 1
    fi
    sleep 0.5
  done
  return 1
}
