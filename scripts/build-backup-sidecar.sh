#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKUP_PROJECT_DIR="$ROOT_DIR/backup_project"
BUILD_DIR="$BACKUP_PROJECT_DIR/build"
TAURI_BIN_DIR="$ROOT_DIR/tauri-app/src-tauri/bin"

if ! command -v rustc >/dev/null 2>&1; then
  echo "error: rustc not found; install Rust before building the sidecar." >&2
  exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "error: cmake not found; install CMake before building the sidecar." >&2
  exit 1
fi

if ! command -v make >/dev/null 2>&1; then
  echo "error: make not found; install build-essential before building the sidecar." >&2
  exit 1
fi

HOST_TRIPLE="$(rustc -vV | sed -n 's/^host: //p')"

if [[ -z "$HOST_TRIPLE" ]]; then
  echo "error: failed to detect Rust host triple." >&2
  exit 1
fi

TARGET_EXT=""
SOURCE_NAME="backup_server"

case "$HOST_TRIPLE" in
  *windows*)
    TARGET_EXT=".exe"
    SOURCE_NAME="backup_server.exe"
    ;;
esac

mkdir -p "$BUILD_DIR" "$TAURI_BIN_DIR"

echo "==> Building backup_project sidecar for $HOST_TRIPLE"
cmake -S "$BACKUP_PROJECT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --parallel

SOURCE_PATH="$BUILD_DIR/$SOURCE_NAME"
TARGET_PATH="$TAURI_BIN_DIR/backup_server-$HOST_TRIPLE$TARGET_EXT"

if [[ ! -f "$SOURCE_PATH" ]]; then
  echo "error: built sidecar not found at $SOURCE_PATH" >&2
  exit 1
fi

cp "$SOURCE_PATH" "$TARGET_PATH"

if [[ "$TARGET_EXT" != ".exe" ]]; then
  chmod +x "$TARGET_PATH"
fi

echo "==> Sidecar ready at $TARGET_PATH"
