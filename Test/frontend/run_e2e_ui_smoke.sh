#!/usr/bin/env bash

set -u

TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$TEST_ROOT/.." && pwd)"
OUTPUT_ROOT="$TEST_ROOT/output/frontend"
RUN_ID="ui-smoke-$(date +%Y%m%d_%H%M%S)-$$"
RUN_DIR="$OUTPUT_ROOT/$RUN_ID"
export PATH="$HOME/.cargo/bin:$PATH"
export NODE_PATH="$REPO_ROOT/tauri-app/node_modules"
export BACKUP_PORT=8080
export BACKUP_DB="$RUN_DIR/users.db"
export BACKUP_CLOUD_DIR="$RUN_DIR/cloud"

mkdir -p "$RUN_DIR" "$BACKUP_CLOUD_DIR"

vite_pid=""
backup_server_path="$REPO_ROOT/tauri-app/src-tauri/target/debug/backup_server"

port_pids() {
  local port="$1"
  ss -ltnp "sport = :$port" 2>/dev/null | sed -n 's/.*pid=\([0-9][0-9]*\).*/\1/p'
}

cleanup_e2e_sidecar() {
  local pid cmd
  while read -r pid; do
    [[ -n "$pid" ]] || continue
    cmd="$(ps -p "$pid" -o args= 2>/dev/null || true)"
    if [[ "$cmd" == "$backup_server_path"* ]]; then
      kill "$pid" 2>/dev/null || true
      wait "$pid" 2>/dev/null || true
    fi
  done < <(port_pids 8080)
}

cleanup() {
  cleanup_e2e_sidecar
  if [[ -n "$vite_pid" ]] && kill -0 "$vite_pid" 2>/dev/null; then
    kill -- "-$vite_pid" 2>/dev/null || kill "$vite_pid" 2>/dev/null || true
    wait "$vite_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT

wait_frontend() {
  local deadline=$((SECONDS + 30))
  while (( SECONDS < deadline )); do
    if curl -fsS "http://127.0.0.1:1420" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.2
  done
  return 1
}

wait_port_closed() {
  local port="$1"
  local timeout_seconds="$2"
  local deadline=$((SECONDS + timeout_seconds))
  while (( SECONDS < deadline )); do
    if ! timeout 1 bash -c ":</dev/tcp/127.0.0.1/$port" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.5
  done
  return 1
}

wait_port_available() {
  local port="$1"
  local timeout_seconds="$2"
  wait_port_closed "$port" "$timeout_seconds"
}

bash "$TEST_ROOT/frontend/check_e2e_env.sh" || exit $?

cd "$REPO_ROOT/tauri-app"
if ! wait_port_available 1420 60; then
  echo "BLOCKED port 1420 is already in use after waiting; E2E will not kill unknown processes." >&2
  exit 77
fi
setsid npm run dev >"$OUTPUT_ROOT/vite-ui-smoke.log" 2>&1 &
vite_pid=$!
printf '%s\n' "$vite_pid" >"$OUTPUT_ROOT/vite-ui-smoke.pid"
wait_frontend || {
  echo "FAIL Vite frontend did not become reachable on 127.0.0.1:1420" >&2
  exit 1
}
status=0
npm run test:e2e:tauri:ui-smoke || status=$?
cleanup
if ! wait_port_closed 1420 60; then
  echo "BLOCKED port 1420 did not release after UI smoke E2E; no unknown process was killed." >&2
  status=77
fi
if ! wait_port_closed 8080 120; then
  echo "BLOCKED port 8080 did not release after UI smoke E2E; no unknown process was killed." >&2
  status=77
fi
exit "$status"
