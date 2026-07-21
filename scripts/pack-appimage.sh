#!/usr/bin/env bash
# 在含中文路径的工程里，tauri 自带的 appimagetool 容易卡住（下载 runtime / 非 ASCII 路径）。
# 本脚本：先让 tauri 生成 AppDir（或复用已有），再在 /tmp 纯英文路径下本地压包。

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TAURI_APP="$ROOT_DIR/tauri-app"
BUNDLE_DIR="$TAURI_APP/src-tauri/target/release/bundle/appimage"
APPDIR_NAME="Data-Backup.AppDir"
APPIMAGE_NAME="Data-Backup_0.1.0_amd64.AppImage"
WORK="/tmp/data-backup-appimage-work"
RUNTIME_SRC="$HOME/.cache/tauri/linuxdeploy-plugin-appimage.AppImage"

SKIP_TAURI=0
if [[ "${1:-}" == "--reuse-appdir" ]]; then
  SKIP_TAURI=1
fi

if [[ ! -x "$RUNTIME_SRC" ]]; then
  echo "error: missing $RUNTIME_SRC — run once: cd tauri-app && npm run tauri build" >&2
  echo "       (even if it hangs later, linuxdeploy tools will be cached)" >&2
  exit 1
fi

mkdir -p "$WORK"
rm -rf "$WORK/AppDir" "$WORK/out" "$WORK/tool" "$WORK/runtime-x86_64"
mkdir -p "$WORK/out" "$WORK/tool"

if [[ "$SKIP_TAURI" -eq 0 ]]; then
  echo "==> Building AppDir via tauri (may look idle while linuxdeploy copies libs)..."
  cd "$TAURI_APP"
  # 若上次卡在 appimagetool，先清掉半成品再生成 AppDir
  rm -rf "$BUNDLE_DIR"
  # 超时保护：若长时间卡在 appimagetool，用户可 Ctrl+C 后改用 --reuse-appdir
  NO_STRIP=true APPIMAGE_EXTRACT_AND_RUN=1 npm run tauri build || true
fi

if [[ ! -d "$BUNDLE_DIR/$APPDIR_NAME" ]]; then
  echo "error: AppDir not found at $BUNDLE_DIR/$APPDIR_NAME" >&2
  echo "hint: run without --reuse-appdir once, wait until AppDir appears, then Ctrl+C and rerun with --reuse-appdir" >&2
  exit 1
fi

echo "==> Copying AppDir to ASCII path $WORK/AppDir"
cp -a "$BUNDLE_DIR/$APPDIR_NAME" "$WORK/AppDir"

# linuxdeploy 有时不打包 WebKit 子进程，导致 AppImage 白屏/直接退出
if [[ -d /usr/libexec/libwebkit2gtk-4_1-0 ]]; then
  echo "==> Bundling WebKit process helpers"
  mkdir -p "$WORK/AppDir/usr/libexec"
  cp -a /usr/libexec/libwebkit2gtk-4_1-0 "$WORK/AppDir/usr/libexec/"
  [[ -e "$WORK/AppDir/libexec" ]] || ln -s usr/libexec "$WORK/AppDir/libexec"
fi

echo "==> Extracting local type2 runtime from cached plugin AppImage"
python3 - "$RUNTIME_SRC" "$WORK/runtime-x86_64" <<'PY'
import sys
from pathlib import Path
src, out = map(Path, sys.argv[1:3])
data = src.read_bytes()
# Prefer the last squashfs magic after ELF payload (type2 runtime boundary)
idxs = []
start = 0
while True:
    i = data.find(b"hsqs", start)
    if i < 0:
        break
    idxs.append(i)
    start = i + 1
if not idxs:
    raise SystemExit("no squashfs magic in plugin AppImage")
# For plugin AppImage, first hsqs after ~100k is usually the runtime|payload split
idx = next((i for i in idxs if i > 100000), idxs[-1])
out.write_bytes(data[:idx])
print(f"runtime bytes={out.stat().st_size} offset={idx}")
PY

echo "==> Extracting appimagetool"
cd "$WORK/tool"
APPIMAGE_EXTRACT_AND_RUN=1 "$RUNTIME_SRC" --appimage-extract >/dev/null
TOOL="$WORK/tool/squashfs-root/usr/bin/appimagetool"
chmod +x "$TOOL"

echo "==> Packing AppImage"
export ARCH=x86_64
export APPIMAGE_EXTRACT_AND_RUN=1
"$TOOL" -n -v --runtime-file "$WORK/runtime-x86_64" \
  "$WORK/AppDir" "$WORK/out/$APPIMAGE_NAME"

mkdir -p "$BUNDLE_DIR"
cp -f "$WORK/out/$APPIMAGE_NAME" "$BUNDLE_DIR/$APPIMAGE_NAME"
chmod +x "$BUNDLE_DIR/$APPIMAGE_NAME"

echo "==> Done: $BUNDLE_DIR/$APPIMAGE_NAME"
ls -lh "$BUNDLE_DIR/$APPIMAGE_NAME"
