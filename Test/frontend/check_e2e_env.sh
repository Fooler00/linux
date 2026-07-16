#!/usr/bin/env bash

set -u

TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$TEST_ROOT/.." && pwd)"
OUTPUT_ROOT="$TEST_ROOT/output/frontend"
mkdir -p "$OUTPUT_ROOT"
export PATH="$HOME/.cargo/bin:$PATH"

missing=0

check_cmd() {
  if command -v "$1" >/dev/null 2>&1; then
    printf 'PASS e2e dependency present: %s\n' "$1"
  else
    printf 'BLOCKED missing e2e dependency: %s\n' "$1"
    missing=1
  fi
}

check_cmd node
check_cmd npm
check_cmd cargo
check_cmd tauri-driver

if command -v WebKitWebDriver >/dev/null 2>&1; then
  printf 'PASS e2e dependency present: WebKitWebDriver\n'
elif command -v webkit2gtk-driver >/dev/null 2>&1; then
  printf 'PASS e2e dependency present: webkit2gtk-driver\n'
else
  printf 'BLOCKED missing e2e dependency: WebKitWebDriver/webkit2gtk-driver\n'
  missing=1
fi

check_cmd Xvfb

port8080_busy() {
  timeout 1 bash -c ':</dev/tcp/127.0.0.1/8080' >/dev/null 2>&1
}

if port8080_busy; then
  deadline=$((SECONDS + 60))
  while (( SECONDS < deadline )) && port8080_busy; do
    sleep 0.5
  done
fi

if port8080_busy; then
  printf 'BLOCKED port 8080 is already in use by an unknown process after waiting; Tauri E2E will not kill it.\n'
  missing=1
else
  printf 'PASS port 8080 is free before Tauri E2E\n'
fi

if (( missing != 0 )); then
  cat >"$OUTPUT_ROOT/e2e_blocked.txt" <<'MSG'
Tauri E2E is blocked by missing system dependencies or port 8080.

Debian/Ubuntu install commands:
  sudo apt-get update
  sudo apt-get install -y webkit2gtk-driver xvfb

Command meanings:
  apt-get update: refresh package indexes.
  apt-get install webkit2gtk-driver: install Linux WebKit WebDriver.
  apt-get install xvfb: install virtual X server for headless GUI tests.
MSG
  exit 77
fi
