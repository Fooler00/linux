#!/usr/bin/env bash

set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/scripts/common.sh"

for cmd in cmake ctest g++ curl tar timeout; do
  require_command "$cmd"
  pass "environment has $cmd"
done

for optional in zip unzip openssl sqlite3 jq valgrind tauri-driver WebKitWebDriver Xvfb node npm; do
  if command -v "$optional" >/dev/null 2>&1; then
    pass "optional tool present: $optional"
  else
    printf 'BLOCKED optional tool missing: %s\n' "$optional"
  fi
done
