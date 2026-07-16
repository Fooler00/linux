#!/usr/bin/env bash

set -u

cat <<'MSG'
BLOCKED Frontend full business interaction E2E is intentionally excluded from default regression.

Reason:
  TEST-INFRA-001 documents WebKitWebDriver/Tauri session timeouts observed during
  complex input, click, executeAsyncScript, visibility and status-query commands.

Current default frontend coverage:
  - Frontend.E2EEnv checks system dependencies and port readiness.
  - Frontend.E2ESmoke performs real AuthPanel registration input/click/assertion.
  - Frontend.E2EUISmoke performs tab navigation and page/control visibility smoke.

Do not treat UI smoke as full backup/restore/manage/cloud/task business success.
MSG

exit 77
