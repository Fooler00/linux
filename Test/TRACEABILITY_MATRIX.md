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
| TC-013 | User login/logout | `UserAuthTaskUnit.UserManager_RegisterAndAuthenticateWithTempSqlite`, `Frontend.E2ESmoke` | C++ PASS, E2E BLOCKED by missing WebKitWebDriver/Xvfb |
| TC-014 | TAR archive backup and restore | `ArchiveCryptoComponent.TarArchive_RoundTripsDirectory`, `CLI.Flow` | PASS |
| TC-015 | TAR.GZ archive backup and restore | `ArchiveCryptoComponent.TarGzArchive_RoundTripsDirectory`, `CLI.Flow` | PASS |
| TC-016 | Metadata preservation | `BackupRestoreIntegration.SourceFile_IsNotModifiedByBackup`, `MetadataManifestIntegration.MetadataJson_RecordsSourceAndOptions` | PASS |
| TC-017 | Special files / symlink | `DefectReproductionIntegration.SymlinkBackup_PreservesLinkWhenSupported` | PASS |
| TC-018 | Wrong password restore | `ArchiveCryptoComponent.EncryptDecrypt_WrongPasswordThrows`, `BackupRestoreIntegration.EncryptedBackup_WrongPasswordThrows`, `Security.Basic` | PASS |
| TC-019 | Cloud upload and list | `CloudStorageComponent.UploadListDownloadAndDelete`, `API.WatchScheduleCloud` | PASS |
| TC-020 | Cloud download | `CloudStorageComponent.UploadListDownloadAndDelete`, `API.WatchScheduleCloud` | PASS |
| TC-021 | Cloud delete | `CloudStorageComponent.UploadListDownloadAndDelete`, `API.WatchScheduleCloud` | PASS |

TC-012 conclusion: `历史报告编号缺失，原用例不存在，不迁移`.
