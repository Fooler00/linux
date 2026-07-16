# Traceability Matrix

Historical results are not reused. Each historical TC is mapped to current `v1-test` automated coverage.

| Historical TC | Historical purpose | Current coverage | Current status |
|---|---|---|---|
| TC-001 | Normal backup and restore | `BackupRestoreIntegration.DirectoryBackupRestore_PreservesNestedContent`, `CLI.Flow`, `API.Core` | PASS |
| TC-002 | ZIP archive backup and restore | `ArchiveCryptoComponent.ZipArchive_RoundTripsDirectory`, `BackupRestoreIntegration.ZipBackupRestore_RoundTrips`, `CLI.Flow` | PASS |
| TC-003A | `*.log` advanced filter | `FilterUnit.GlobLogPattern_MatchesVisibleLogFile`, `DefectReproductionIntegration.GlobLogFilter_IncludesVisibleLogFiles` | PASS |
| TC-003B | `.log` compatibility filter | `FilterUnit.GlobLogPattern_CurrentlyMatchesDotLogCompatibility`, `DefectReproductionIntegration.DotLogCompatibility_RequiresExplicitDotPattern` | PASS |
| TC-004 | Encrypted backup and correct password restore | `ArchiveCryptoComponent.EncryptDecrypt_RoundTripsFile`, `Security.Basic` | PASS |
| TC-005 | Incremental backup | `MetadataManifestIntegration.IncrementalNoChanges_WritesIncskipMarkers`, `IncrementalModifiedFile_CopiesChangedContent`, `IncrementalNewFile_CopiesNewFile` | PASS |
| TC-006 | Backup management list | `DefectReproductionIntegration.BackupList_ReturnsCreatedBackups`, `API.Core` | PASS |
| TC-007 | Metadata view | `MetadataManifestIntegration.MetadataJson_RecordsSourceAndOptions`, `API.Core` | PASS |
| TC-008 | Backup prune strategy | `CLI.Flow`, `API.Core`, `API.WatchScheduleCloud` schedule maxBackups subcase | PASS |
| TC-009 | Missing source path | `BackupRestoreIntegration.MissingSource_Throws`, `CLI.Flow`, `API.Core` | PASS |
| TC-010 | Realtime watch backup | `API.WatchScheduleCloud` | FAIL; `DEF-001` stable, 10/10 strict reruns backed up a sentinel file created after watch stop response |
| TC-011 | Scheduled backup and retention | `API.WatchScheduleCloud` | PASS |
| TC-012 | Historical number gap | Original report has no TC-012 entry | NOT_RUN |
| TC-013 | User login/logout | `UserAuthTaskUnit.UserManager_RegisterAndAuthenticateWithTempSqlite`, `Frontend.E2ESmoke` | PASS |
| TC-014 | TAR archive backup and restore | `ArchiveCryptoComponent.TarArchive_RoundTripsDirectory`, `CLI.Flow` | PASS |
| TC-015 | TAR.GZ archive backup and restore | `ArchiveCryptoComponent.TarGzArchive_RoundTripsDirectory`, `CLI.Flow` | PASS |
| TC-016 | Metadata preservation | `BackupRestoreIntegration.SourceFile_IsNotModifiedByBackup`, `MetadataManifestIntegration.MetadataJson_RecordsSourceAndOptions` | PASS |
| TC-017 | Special files / symlink | `DefectReproductionIntegration.SymlinkBackup_PreservesLinkWhenSupported` | PASS |
| TC-018 | Wrong password restore | `ArchiveCryptoComponent.EncryptDecrypt_WrongPasswordThrows`, `BackupRestoreIntegration.EncryptedBackup_WrongPasswordThrows`, `Security.Basic` | PASS |
| TC-019 | Cloud upload and list | `CloudStorageComponent.UploadListDownloadAndDelete`, `API.WatchScheduleCloud` | PASS |
| TC-020 | Cloud download | `CloudStorageComponent.UploadListDownloadAndDelete`, `API.WatchScheduleCloud` | PASS |
| TC-021 | Cloud delete | `CloudStorageComponent.UploadListDownloadAndDelete`, `API.WatchScheduleCloud` | PASS |

TC-012 conclusion: `历史报告编号缺失，原用例不存在，不迁移`.

## Frontend Coverage Classification

| Frontend area | Current automated coverage | Status |
|---|---|---|
| AuthPanel | Real E2E registration input, click, and workspace assertion in `Frontend.E2ESmoke` | PASS |
| BackupPanel | UI navigation/page smoke only; full backup business flow covered by C++/CLI/API tests | UI smoke PASS; full frontend business E2E BLOCKED/NOT_RUN |
| RestorePanel | UI navigation/page smoke only; full restore business flow covered by C++/CLI/API tests | UI smoke PASS; full frontend business E2E BLOCKED/NOT_RUN |
| WatchPanel | UI controls smoke; watch behavior covered by `API.WatchScheduleCloud` | UI smoke PASS; API has `DEF-001` FAIL |
| SchedulePanel | UI controls smoke; schedule behavior covered by `API.WatchScheduleCloud` | UI smoke PASS; API schedule subcases PASS |
| BackupManagePanel | UI navigation/page smoke only; list/prune behavior covered by integration/CLI/API | UI smoke PASS; full frontend business E2E BLOCKED/NOT_RUN |
| CloudPanel | UI navigation/page smoke only; cloud behavior covered by component/API tests | UI smoke PASS; full frontend business E2E BLOCKED/NOT_RUN |
| TaskList | UI navigation/page smoke only; task behavior covered by unit/API tests | UI smoke PASS; full frontend business E2E BLOCKED/NOT_RUN |

The frontend UI smoke checks are not counted as complete business-flow passes. `TEST-INFRA-001` tracks the WebKitWebDriver/Tauri stability limitation that currently blocks full frontend business E2E.
