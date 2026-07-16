#!/usr/bin/env bash

set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/scripts/common.sh"

CLI="$BUILD_DIR/backup_cli"
[[ -x "$CLI" ]] || skip "backup_cli not built at $CLI"

src="$RUN_DIR/source"
dst="$RUN_DIR/backups"
restore="$RUN_DIR/restore"
mkdir -p "$src/nested" "$dst" "$restore"
printf 'alpha' >"$src/a.txt"
printf 'beta' >"$src/nested/b.txt"

out="$("$CLI" backup "$src" "$dst" --algo none 2>&1)"
status=$?
(( status == 0 )) && pass "CLI backup --algo none succeeds" || fail "CLI backup --algo none succeeds: $out"
backup_path="$(printf '%s\n' "$out" | sed -n 's/.*备份完成: "\(.*\)"/\1/p')"
[[ -n "$backup_path" ]] || backup_path="$(find "$dst" -maxdepth 1 -type d -name 'backup_*' | head -1)"
assert_file_exists "$backup_path/metadata.json" "CLI backup writes metadata.json"
assert_file_exists "$backup_path/manifest.json" "CLI backup writes manifest.json"

out="$("$CLI" restore "$backup_path" "$restore" 2>&1)"
status=$?
(( status == 0 )) && pass "CLI restore succeeds" || fail "CLI restore succeeds: $out"
restored_root="$(find "$restore" -maxdepth 1 -type d -name 'backup_*' | head -1)"
assert_equal_file "$src/a.txt" "$restored_root/a.txt" "CLI restore preserves root file"
assert_equal_file "$src/nested/b.txt" "$restored_root/nested/b.txt" "CLI restore preserves nested file"

list_out="$("$CLI" list "$dst" 2>&1)"
assert_contains "$list_out" "backup_" "CLI list shows backup entry"

prune_out="$("$CLI" prune "$dst" --max-backups 5 --max-age-days 0 2>&1)"
(( $? == 0 )) && pass "CLI prune command succeeds" || fail "CLI prune command succeeds: $prune_out"

bad_out="$("$CLI" backup "$RUN_DIR/missing" "$dst" --algo none 2>&1)"
(( $? != 0 )) && pass "CLI missing source fails" || fail "CLI missing source fails"

if command -v zip >/dev/null 2>&1 && command -v unzip >/dev/null 2>&1; then
  zip_src="$RUN_DIR/zip source"
  zip_dst="$RUN_DIR/zip backups"
  zip_restore="$RUN_DIR/zip restore"
  mkdir -p "$zip_src" "$zip_dst" "$zip_restore"
  printf 'zip-content' >"$zip_src/file.txt"
  zip_out="$("$CLI" backup "$zip_src" "$zip_dst" --algo zip 2>&1)"
  (( $? == 0 )) && pass "CLI zip backup succeeds" || fail "CLI zip backup succeeds: $zip_out"
  zip_path="$(find "$zip_dst" -maxdepth 1 -type f -name 'backup_*.zip' | head -1)"
  assert_file_exists "$zip_path" "CLI zip archive exists"
  "$CLI" restore "$zip_path" "$zip_restore" >/dev/null 2>&1
  found_zip="$(find "$zip_restore" -type f -name 'file.txt' | head -1)"
  assert_equal_file "$zip_src/file.txt" "$found_zip" "CLI zip restore preserves content"
else
  pass "CLI zip scenario skipped because zip/unzip is unavailable"
fi

if command -v tar >/dev/null 2>&1; then
  tar_src="$RUN_DIR/tar-source"
  tar_dst="$RUN_DIR/tar-backups"
  mkdir -p "$tar_src" "$tar_dst"
  printf 'tar-content' >"$tar_src/file.txt"
  "$CLI" backup "$tar_src" "$tar_dst" --algo tar >/dev/null 2>&1
  assert_file_exists "$(find "$tar_dst" -maxdepth 1 -type f -name 'backup_*.tar' | head -1)" "CLI tar archive exists"

  tgz_src="$RUN_DIR/tgz-source"
  tgz_dst="$RUN_DIR/tgz-backups"
  mkdir -p "$tgz_src" "$tgz_dst"
  printf 'tgz-content' >"$tgz_src/file.txt"
  "$CLI" backup "$tgz_src" "$tgz_dst" --algo tar.gz >/dev/null 2>&1
  assert_file_exists "$(find "$tgz_dst" -maxdepth 1 -type f -name 'backup_*.tar.gz' | head -1)" "CLI tar.gz archive exists"
else
  pass "CLI tar/tar.gz scenarios skipped because tar is unavailable"
fi

if command -v openssl >/dev/null 2>&1; then
  enc_src="$RUN_DIR/encrypted-source"
  enc_dst="$RUN_DIR/encrypted-backups"
  enc_restore="$RUN_DIR/encrypted-restore"
  mkdir -p "$enc_src" "$enc_dst" "$enc_restore"
  printf 'secret' >"$enc_src/secret.txt"
  "$CLI" backup "$enc_src" "$enc_dst" --algo none --encrypt correct-password >/dev/null 2>&1
  enc_path="$(find "$enc_dst" -maxdepth 1 -type f -name 'backup_*.enc' | head -1)"
  assert_file_exists "$enc_path" "CLI encrypted backup exists"
  "$CLI" restore "$enc_path" "$enc_restore" --password wrong-password >/dev/null 2>&1
  (( $? != 0 )) && pass "CLI wrong password restore fails" || fail "CLI wrong password restore fails"
else
  pass "CLI encryption scenario skipped because openssl is unavailable"
fi
