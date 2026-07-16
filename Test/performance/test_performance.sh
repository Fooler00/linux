#!/usr/bin/env bash

set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/scripts/common.sh"

CLI="$BUILD_DIR/backup_cli"
[[ -x "$CLI" ]] || skip "backup_cli not built at $CLI"

src="$RUN_DIR/perf-source"
dst="$RUN_DIR/perf-backups"
restore="$RUN_DIR/perf-restore"
mkdir -p "$src/small" "$dst" "$restore"

for i in $(seq -w 1 1000); do
  printf 'file-%s-%01024d\n' "$i" 0 >"$src/small/file_$i.txt"
done
dd if=/dev/zero of="$src/single_4mb.bin" bs=1M count=4 status=none

result="$RUN_DIR/performance.tsv"
printf 'scenario\tstart_epoch\tend_epoch\tduration_ms\tstatus\toutput_size\n' >"$result"

run_perf() {
  local name="$1"
  shift
  local start end status size
  start="$(date +%s%3N)"
  "$@" >/dev/null 2>"$RUN_DIR/${name}.log"
  status=$?
  end="$(date +%s%3N)"
  size="$(du -sb "$dst" 2>/dev/null | awk '{print $1}')"
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$name" "$start" "$end" "$((end - start))" "$status" "${size:-0}" >>"$result"
  (( status == 0 )) && pass "Performance $name command succeeds" || fail "Performance $name command succeeds"
}

run_perf backup_none "$CLI" backup "$src" "$dst/none" --algo none
backup_none="$(find "$dst/none" -maxdepth 1 -name 'backup_*' | head -1)"
mkdir -p "$restore/none"
run_perf restore_none "$CLI" restore "$backup_none" "$restore/none"

if command -v zip >/dev/null 2>&1 && command -v unzip >/dev/null 2>&1; then
  run_perf backup_zip "$CLI" backup "$src" "$dst/zip" --algo zip
else
  pass "Performance zip scenario skipped because zip/unzip is unavailable"
fi

if command -v openssl >/dev/null 2>&1; then
  run_perf backup_encrypted "$CLI" backup "$src" "$dst/encrypted" --algo none --encrypt perf-password
else
  pass "Performance encrypted scenario skipped because openssl is unavailable"
fi

run_perf backup_incremental_base "$CLI" backup "$src" "$dst/incremental-base" --algo none
base="$(find "$dst/incremental-base" -maxdepth 1 -name 'backup_*' | head -1)"
printf 'changed' >"$src/small/file_0001.txt"
start="$(date +%s%3N)"
"$CLI" backup "$src" "$dst/incremental-next" --algo none >/dev/null 2>"$RUN_DIR/backup_incremental_next.log"
status=$?
end="$(date +%s%3N)"
printf '%s\t%s\t%s\t%s\t%s\t%s\n' "backup_incremental_next_cli_no_flag" "$start" "$end" "$((end - start))" "$status" "0" >>"$result"
(( status == 0 )) && pass "Performance incremental reference command succeeds" || fail "Performance incremental reference command succeeds"

assert_file_exists "$result" "Performance result file written"
