#!/usr/bin/env bash

set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/scripts/common.sh"

CLI="$BUILD_DIR/backup_cli"
[[ -x "$CLI" ]] || skip "backup_cli not built at $CLI"

src="$RUN_DIR/security source"
dst="$RUN_DIR/security backups"
restore="$RUN_DIR/security restore"
mkdir -p "$src" "$dst" "$restore"
printf 'safe' >"$src/space file.txt"
printf 'semi' >"$src/semi;colon.txt"
printf 'quote' >"$src/single'quote.txt"
printf 'double' >"$src/double\"quote.txt"
printf 'dollar' >"$src/dollar\$(touch should_not_exist).txt"
printf '中文' >"$src/中文 文件.txt"

"$CLI" backup "$src" "$dst" --algo none >/dev/null 2>"$RUN_DIR/security_backup.log"
(( $? == 0 )) && pass "Security special-character backup succeeds" || fail "Security special-character backup succeeds"
backup_path="$(find "$dst" -maxdepth 1 -name 'backup_*' | head -1)"
"$CLI" restore "$backup_path" "$restore" >/dev/null 2>"$RUN_DIR/security_restore.log"
(( $? == 0 )) && pass "Security special-character restore succeeds" || fail "Security special-character restore succeeds"
assert_file_not_exists "$REPO_ROOT/should_not_exist" "Security shell metacharacter filename is not executed"

if command -v openssl >/dev/null 2>&1; then
  enc_src="$RUN_DIR/sec-enc-source"
  enc_dst="$RUN_DIR/sec-enc-backups"
  mkdir -p "$enc_src" "$enc_dst"
  printf 'secret' >"$enc_src/secret.txt"
  "$CLI" backup "$enc_src" "$enc_dst" --algo none --encrypt correct >/dev/null 2>&1
  enc_path="$(find "$enc_dst" -maxdepth 1 -name 'backup_*.enc' | head -1)"
  "$CLI" restore "$enc_path" "$RUN_DIR/sec-wrong-restore" --password wrong >/dev/null 2>&1
  (( $? != 0 )) && pass "Security wrong password restore fails" || fail "Security wrong password restore fails"
else
  pass "Security wrong password scenario skipped because openssl is unavailable"
fi

printf 'not a real archive' >"$RUN_DIR/corrupt.zip"
"$CLI" restore "$RUN_DIR/corrupt.zip" "$RUN_DIR/corrupt-restore" >/dev/null 2>&1
(( $? != 0 )) && pass "Security corrupt archive restore fails" || fail "Security corrupt archive restore fails"

printf '{bad json' >"$RUN_DIR/corrupt_manifest.json"
assert_file_exists "$RUN_DIR/corrupt_manifest.json" "Security damaged JSON fixture isolated in output"

if ln -s "../outside.txt" "$src/link_escape" 2>/dev/null; then
  printf 'outside' >"$RUN_DIR/outside.txt"
  "$CLI" backup "$src" "$RUN_DIR/symlink-backups" --algo none >/dev/null 2>&1
  (( $? == 0 )) && pass "Security symlink backup completes without system mutation" || fail "Security symlink backup completes without system mutation"
else
  pass "Security symlink scenario skipped because symlink creation failed"
fi

if grep -R "correct" "$RUN_DIR"/*.log >/dev/null 2>&1; then
  fail "Security password leaked to test logs"
else
  pass "Security password not present in test logs"
fi
