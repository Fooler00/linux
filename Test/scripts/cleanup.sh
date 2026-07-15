#!/usr/bin/env bash

set -u

TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_ROOT="$TEST_ROOT/output"

find "$OUTPUT_ROOT" -name '*.pid' -type f | while read -r pidfile; do
  pid="$(cat "$pidfile" 2>/dev/null || true)"
  if [[ "$pid" =~ ^[0-9]+$ ]] && kill -0 "$pid" 2>/dev/null; then
    printf 'Recorded test process still running: %s from %s\n' "$pid" "$pidfile" >&2
  fi
done
