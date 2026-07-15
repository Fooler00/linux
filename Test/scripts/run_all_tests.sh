#!/usr/bin/env bash

set -u

TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$TEST_ROOT/.." && pwd)"

cmake -S "$REPO_ROOT/backup_project" -B "$TEST_ROOT/build/cpp" -DBUILD_TESTING=ON
cmake --build "$TEST_ROOT/build/cpp" --parallel
ctest --test-dir "$TEST_ROOT/build/cpp" --output-on-failure
