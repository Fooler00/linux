# Test Cases

Each CTest entry maps to a concrete test file or script.

## C++ Unit

| ID | Layer | Test name | Object | File | Expected |
|---|---|---|---|---|---|
| UT-001 | unit | `UtilsUnit.BytesToHex_ConvertsLowercaseHex` | `bytesToHex` | `unit/test_utils.cpp` | PASS |
| UT-002 | unit | `UtilsUnit.Sha256Hex_KnownDigest` | `sha256Hex` | `unit/test_utils.cpp` | PASS |
| UT-003 | unit | `UtilsUnit.RandomHex_ReturnsTwoCharsPerByte` | `randomHex` | `unit/test_utils.cpp` | PASS |
| UT-004 | unit | `UtilsUnit.HashPassword_IsStableForSameSalt` | `hashPassword` | `unit/test_utils.cpp` | PASS |
| UT-005 | unit | `UtilsUnit.ShellQuote_QuotesShellMetacharacters` | `shellQuote` | `unit/test_utils.cpp` | PASS |
| UT-006 | unit | `UtilsUnit.ParseTimeString_AcceptsDateTime` | `parseTimeString` | `unit/test_utils.cpp` | PASS |
| UT-007 | unit | `UtilsUnit.DirectorySize_SumsNestedFiles` | `directorySize` | `unit/test_utils.cpp` | PASS |
| UT-008 | unit | `FilterUnit.ParseEmptyFilter_UsesTopLevelOptions` | `parseBackupFilter` | `unit/test_filter.cpp` | PASS |
| UT-009 | unit | `FilterUnit.ParseExtensions_AddsLeadingDot` | `parseBackupFilter` | `unit/test_filter.cpp` | PASS |
| UT-010 | unit | `FilterUnit.ExtensionFilter_MatchesOnlyConfiguredExtensions` | `fileMatchesFilter` | `unit/test_filter.cpp` | PASS |
| UT-011 | unit | `FilterUnit.GlobLogPattern_MatchesVisibleLogFile` | `fileMatchesFilter` | `unit/test_filter.cpp` | PASS |
| UT-012 | unit | `FilterUnit.GlobLogPattern_CurrentlyMatchesDotLogCompatibility` | `fileMatchesFilter` | `unit/test_filter.cpp` | PASS |
| UT-013 | unit | `FilterUnit.ExcludePath_OverridesInclude` | `fileMatchesFilter` | `unit/test_filter.cpp` | PASS |
| UT-014 | unit | `FilterUnit.FileNameContains_FiltersByFilename` | `fileMatchesFilter` | `unit/test_filter.cpp` | PASS |
| UT-015 | unit | `FilterUnit.SizeBounds_AcceptOnlyWithinRange` | `fileMatchesFilter` | `unit/test_filter.cpp` | PASS |
| UT-016 | unit | `FilterUnit.UnicodeAndSpacePath_MatchesLexicalRelativePath` | `fileMatchesFilter` | `unit/test_filter.cpp` | PASS |
| UT-017 | unit | `ManifestUnit.MissingManifest_ReturnsEmptyMap` | `loadIncrementalManifest` | `unit/test_incremental_manifest.cpp` | PASS |
| UT-018 | unit | `ManifestUnit.WriteAndLoadManifest_RoundTripsHashes` | manifest read/write | `unit/test_incremental_manifest.cpp` | PASS |
| UT-019 | unit | `ManifestUnit.CorruptManifest_CurrentCompatibilityReturnsEmpty` | manifest parse | `unit/test_incremental_manifest.cpp` | PASS |
| UT-020 | unit | `ManifestUnit.EmptyManifestObject_ReturnsEmptyMap` | manifest parse | `unit/test_incremental_manifest.cpp` | PASS |
| UT-021 | unit | `ManifestUnit.ManifestFileName_IsManifestJson` | manifest write | `unit/test_incremental_manifest.cpp` | PASS |
| UT-022 | unit | `UserAuthTaskUnit.UserManager_RegisterAndAuthenticateWithTempSqlite` | `UserManager` | `unit/test_user_auth_task.cpp` | PASS |
| UT-023 | unit | `UserAuthTaskUnit.UserManager_RejectsDuplicateUser` | `UserManager` | `unit/test_user_auth_task.cpp` | PASS |
| UT-024 | unit | `UserAuthTaskUnit.UserManager_RejectsWrongPassword` | `UserManager` | `unit/test_user_auth_task.cpp` | PASS |
| UT-025 | unit | `UserAuthTaskUnit.AuthManager_TokenReturnsOriginalUser` | `AuthManager` | `unit/test_user_auth_task.cpp` | PASS |
| UT-026 | unit | `UserAuthTaskUnit.AuthManager_UnknownTokenReturnsEmptyUser` | `AuthManager` | `unit/test_user_auth_task.cpp` | PASS |
| UT-027 | unit | `UserAuthTaskUnit.TaskManager_AddUpdateAndFilterByUser` | `TaskManager` | `unit/test_user_auth_task.cpp` | PASS |

## Component And Integration

| ID | Layer | File | Count | Coverage |
|---|---|---|---:|---|
| CT-001..CT-009 | component | `component/test_archive_crypto.cpp` | 9 | archive type normalization, zip/tar/tar.gz roundtrip, OpenSSL encryption, wrong password, content hash |
| CT-010..CT-014 | component | `component/test_cloud_storage.cpp` | 5 | LocalCloudStorage type, upload/list/download/delete, Unicode path, missing file, traversal rejection |
| IT-001..IT-008 | integration | `integration/test_backup_restore.cpp` | 8 | backup/restore, empty dir/file, special path names, source protection, missing source, encrypted wrong password, zip restore |
| IT-009..IT-013 | integration | `integration/test_metadata_manifest.cpp` | 5 | metadata, manifest, incremental unchanged/modified/new |
| IT-014..IT-018 | integration | `integration/test_defect_reproduction.cpp` | 5 | `*.log`, `.log`, symlink, list backups, missing restore |

## Flow, Performance, Security And Frontend

| ID | Layer | Test | File | Expected |
|---|---|---|---|---|
| FLOW-001 | cli | `CLI.Flow` | `cli/test_cli.sh` | PASS |
| FLOW-002 | api | `API.Core` | `api/test_api.sh` | PASS |
| FLOW-003 | api/watch/schedule/cloud | `API.WatchScheduleCloud` | `api/test_watch_schedule_cloud.sh` | FAIL: strict two-phase watch-stop check shows a post-stop sentinel file is backed up after successful stop response |
| PERF-001 | performance | `Performance.Basic` | `performance/test_performance.sh` | PASS |
| SEC-001 | security | `Security.Basic` | `security/test_security.sh` | PASS |
| E2E-001 | frontend | `Frontend.E2EEnv` | `frontend/check_e2e_env.sh` | PASS |
| E2E-002 | frontend | `Frontend.E2ESmoke` | `frontend/run_e2e_smoke.sh` | PASS |
| E2E-003 | frontend/ui-smoke | `Frontend.E2EUISmoke` | `frontend/run_e2e_ui_smoke.sh` | PASS: tab navigation and visible page/control smoke only |
| E2E-004 | frontend/full-business | `Frontend.E2EFullBusiness` | `frontend/run_e2e_full_business.sh` | NOT_RUN/BLOCKED: disabled in default CTest because of `TEST-INFRA-001` |

E2E smoke reached WebDriver and executed DOM actions: register tab click, username/password input, register click, and workspace assertions.

`Frontend.E2EUISmoke` reached WebDriver and ran page navigation/UI smoke:

| Flow | Status |
|---|---|
| AuthPanel setup for UI smoke | PASS: registered and entered workspace |
| BackupPanelSmoke_DisplaysBackupForm | PASS |
| RestorePanelSmoke_DisplaysRestoreForm | PASS |
| WatchPanelSmoke_DisplaysWatchControls | PASS |
| SchedulePanelSmoke_DisplaysScheduleControls | PASS |
| BackupManagePanelSmoke_DisplaysManagementControls | PASS |
| CloudPanelSmoke_DisplaysCloudControls | PASS |
| TaskListSmoke_DisplaysTaskListState | PASS |

Full frontend business E2E cases are not counted as passed:

| Flow | Status | Reason |
|---|---|---|
| BackupPanel full backup operation | BLOCKED/NOT_RUN | `TEST-INFRA-001` WebKitWebDriver/Tauri command timeout risk. |
| RestorePanel full restore operation | BLOCKED/NOT_RUN | `TEST-INFRA-001` WebKitWebDriver/Tauri command timeout risk. |
| BackupManagePanel query/prune operation | BLOCKED/NOT_RUN | `TEST-INFRA-001` WebKitWebDriver/Tauri command timeout risk. |
| CloudPanel upload/list/download/delete operation | BLOCKED/NOT_RUN | `TEST-INFRA-001` WebKitWebDriver/Tauri command timeout risk. |
| TaskList status refresh/business verification | BLOCKED/NOT_RUN | `TEST-INFRA-001` WebKitWebDriver/Tauri command timeout risk. |
