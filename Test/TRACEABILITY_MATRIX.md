# 需求-测试追踪矩阵

| 需求/功能 | 页面 | API | 自动化用例 | 测试代码 | 缺陷 |
|---|---|---|---|---|---|
| 用户注册/登录/退出 | AuthPanel | `/api/register`, `/api/login` | FE-AUTH-001..004 | `frontend/specs/auth.e2e.ts` | - |
| 普通备份 | BackupPanel | `/api/backup`, `/api/tasks` | FE-BACKUP-001 | `frontend/specs/backup.e2e.ts` | - |
| ZIP/TAR/TAR.GZ | BackupPanel | `/api/backup` | FE-BACKUP-002 | `frontend/specs/backup.e2e.ts` | - |
| 加密备份 | BackupPanel | `/api/backup` | FE-BACKUP-003 | `frontend/specs/backup.e2e.ts` | - |
| 高级过滤 | BackupPanel | `/api/backup` | FE-BACKUP-006 | `frontend/specs/backup.e2e.ts` | BUG-FILTER-001 |
| 备份还原 | RestorePanel | `/api/restore`, `/api/tasks` | FE-RESTORE-001..004 | `frontend/specs/restore.e2e.ts` | - |
| 实时监听 | WatchPanel | `/api/watch/start`, `/api/watch/stop`, `/api/tasks` | FE-WATCH-001..004 | `frontend/specs/watch.e2e.ts` | UI-WATCH-001 |
| 定时备份 | SchedulePanel | `/api/schedule/start`, `/api/schedule/stop`, `/api/schedules` | FE-SCHEDULE-001..004 | `frontend/specs/schedule.e2e.ts` | - |
| 备份管理/淘汰 | BackupManagePanel | `/api/backups`, `/api/metadata`, `/api/prune` | FE-MANAGE-001..003 | `frontend/specs/manage.e2e.ts` | - |
| 模拟云存储 | CloudPanel | `/api/cloud/upload`, `/api/cloud/list`, `/api/cloud/delete` | FE-CLOUD-001..003 | `frontend/specs/cloud.e2e.ts` | - |
| 任务状态 | TaskList | `/api/tasks` | FE-TASKS-001..002 | `frontend/specs/tasks.e2e.ts` | BUG-TASK-001 |

