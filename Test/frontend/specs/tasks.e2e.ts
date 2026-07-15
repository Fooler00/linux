import path from "node:path";
import { authPage } from "../page-objects/AuthPage";
import { appPage } from "../page-objects/AppPage";
import { backupPage } from "../page-objects/BackupPage";
import { restorePage } from "../page-objects/RestorePage";
import { taskListPage } from "../page-objects/TaskListPage";
import { createBasicSourceTree, createTestDirs, latestBackup } from "../helpers/filesystem";
import { expectVisibleText } from "../helpers/assertions";
import { waitForTask } from "../helpers/server";
import { runId } from "../helpers/env";

describe("TaskList", () => {
  before(async () => {
    await appPage.logoutIfNeeded();
    await authPage.switchToRegister();
    await authPage.fill(`${runId}_tasks`, "pass_123456");
    await authPage.submit("注册并进入");
  });

  it("shows empty, running, success, failed, refreshed, and multi-type task states", async () => {
    await appPage.openTab("任务列表");
    await expectVisibleText("任务列表");

    const dirs = createTestDirs("tasks_flow");
    createBasicSourceTree(dirs.source);

    await appPage.openTab("手动备份");
    await backupPage.fillPaths(dirs.source, dirs.backup);
    await backupPage.submit();
    await appPage.openTab("任务列表");
    await taskListPage.refresh();
    await expectVisibleText("backup");
    await waitForTask((task) => task.type === "backup" && task.status === "success");

    const backup = latestBackup(dirs.backup);
    expect(backup).not.toBeNull();
    await appPage.openTab("备份还原");
    await restorePage.fill(backup!, dirs.restore);
    await restorePage.submit();
    await waitForTask((task) => task.type === "restore" && task.status === "success");

    await appPage.openTab("备份还原");
    await restorePage.fill(path.join(dirs.backup, "missing.zip"), path.join(dirs.restore, "fail"));
    await restorePage.submit();
    await waitForTask((task) => task.type === "restore" && task.status === "failed");

    await appPage.openTab("任务列表");
    await taskListPage.refresh();
    await expectVisibleText("success");
    await expectVisibleText("failed");
    await expectVisibleText("restore");
  });
});
