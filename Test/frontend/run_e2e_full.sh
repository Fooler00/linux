#!/usr/bin/env bash

set -u

TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$TEST_ROOT/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH"

bash "$TEST_ROOT/frontend/check_e2e_env.sh" || exit $?

cd "$REPO_ROOT/tauri-app"
npm run test:e2e:tauri
