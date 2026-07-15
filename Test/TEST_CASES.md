# 测试用例

| 编号 | 类型 | 被测对象 | 输入/步骤 | 预期结果 | 自动化文件 | 缺陷复现 |
|---|---|---|---|---|---|---|
| FE-AUTH-001 | E2E | AuthPanel | 注册新用户 | 进入主界面 | `frontend/specs/auth.e2e.ts` | 否 |
| FE-AUTH-002 | E2E | AuthPanel | 退出后登录 | 登录成功 | `frontend/specs/auth.e2e.ts` | 否 |
| FE-AUTH-003 | E2E | AuthPanel | 错误密码 | 显示错误 | `frontend/specs/auth.e2e.ts` | 否 |
| FE-AUTH-004 | E2E | AuthPanel | 空用户名/密码 | 前端提示必填 | `frontend/specs/auth.e2e.ts` | 否 |
| FE-BACKUP-001 | E2E | BackupPanel | 普通备份 | backup 任务 success | `frontend/specs/backup.e2e.ts` | 否 |
| FE-BACKUP-002 | E2E | BackupPanel | zip/tar/tar.gz | 生成对应备份 | `frontend/specs/backup.e2e.ts` | 否 |
| FE-BACKUP-003 | E2E | BackupPanel | 启用加密 | 生成 `.enc` | `frontend/specs/backup.e2e.ts` | 否 |
| FE-BACKUP-004 | E2E | BackupPanel | 缺少路径 | 前端提示 | `frontend/specs/backup.e2e.ts` | 否 |
| FE-BACKUP-005 | E2E | BackupPanel | 源目录不存在 | failed 任务 | `frontend/specs/backup.e2e.ts` | 否 |
| FE-BACKUP-006 | E2E | BackupPanel | `*.log` 排除 | 当前仍备份 app.log | `frontend/specs/backup.e2e.ts` | 是 |
| FE-RESTORE-001 | E2E | RestorePanel | 普通还原 | restore success | `frontend/specs/restore.e2e.ts` | 否 |
| FE-RESTORE-002 | E2E | RestorePanel | 加密备份正确密码 | restore success | `frontend/specs/restore.e2e.ts` | 否 |
| FE-RESTORE-003 | E2E | RestorePanel | 错误密码 | restore failed | `frontend/specs/restore.e2e.ts` | 否 |
| FE-RESTORE-004 | E2E | RestorePanel | 备份源不存在 | restore failed | `frontend/specs/restore.e2e.ts` | 否 |
| FE-WATCH-001 | E2E | WatchPanel | 启动监听 | 页面显示已启动 | `frontend/specs/watch.e2e.ts` | 否 |
| FE-WATCH-002 | E2E | WatchPanel | 新建/修改/删除文件 | realtime-backup success | `frontend/specs/watch.e2e.ts` | 否 |
| FE-WATCH-003 | E2E | WatchPanel | 重复启动 | 启动按钮禁用 | `frontend/specs/watch.e2e.ts` | 否 |
| FE-WATCH-004 | E2E | WatchPanel | 停止监听 | 页面状态恢复 | `frontend/specs/watch.e2e.ts` | 否 |
| FE-SCHEDULE-001 | E2E | SchedulePanel | 创建定时任务 | 列表出现任务 | `frontend/specs/schedule.e2e.ts` | 否 |
| FE-SCHEDULE-002 | E2E | SchedulePanel | 短间隔执行 | scheduled-backup success | `frontend/specs/schedule.e2e.ts` | 否 |
| FE-SCHEDULE-003 | E2E | SchedulePanel | maxBackups=2 | 最多保留 2 个 | `frontend/specs/schedule.e2e.ts` | 否 |
| FE-SCHEDULE-004 | E2E | SchedulePanel | 非法间隔 | 显示错误 | `frontend/specs/schedule.e2e.ts` | 否 |
| FE-MANAGE-001 | E2E | BackupManagePanel | 查询备份 | 显示列表 | `frontend/specs/manage.e2e.ts` | 否 |
| FE-MANAGE-002 | E2E | BackupManagePanel | 查看 metadata | 显示元数据 | `frontend/specs/manage.e2e.ts` | 否 |
| FE-MANAGE-003 | E2E | BackupManagePanel | 执行淘汰 | 显示淘汰结果 | `frontend/specs/manage.e2e.ts` | 否 |
| FE-CLOUD-001 | E2E | CloudPanel | 上传文件 | 列表出现文件 | `frontend/specs/cloud.e2e.ts` | 否 |
| FE-CLOUD-002 | E2E | CloudPanel | 删除文件 | 列表为空 | `frontend/specs/cloud.e2e.ts` | 否 |
| FE-CLOUD-003 | E2E | CloudPanel | 不存在本地文件 | 显示失败 | `frontend/specs/cloud.e2e.ts` | 否 |
| FE-TASKS-001 | E2E | TaskList | 刷新任务列表 | 显示多类任务 | `frontend/specs/tasks.e2e.ts` | 否 |
| FE-TASKS-002 | E2E | TaskList | 成功/失败任务 | 显示 success/failed | `frontend/specs/tasks.e2e.ts` | 否 |

