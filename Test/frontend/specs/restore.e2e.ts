import path from "node:path";
import fs from "node:fs";
import { authPage } from "../page-objects/AuthPage";
import { appPage } from "../page-objects/AppPage";
import { backupPage } from "../page-objects/BackupPage";
import { restorePage } from "../page-objects/RestorePage";
import { createBasicSourceTree, createTestDirs, latestBackup } from "../helpers/filesystem";
import { expectGlobalMessage } from "../helpers/assertions";
import { waitForTask } from "../helpers/server";
import { runId } from "../helpers/env";

async function login() {
  await appPage.logoutIfNeeded();
  await authPage.switchToRegister();
  await authPage.fill(`${runId}_restore`, "pass_123456");
  await authPage.submit("注册并进入");
}

describe("RestorePanel", () => {
  before(async () => {
    await login();
  });

  it("restores normal and encrypted backups and reports wrong password failures", async () => {
    const dirs = createTestDirs("restore_flow");
    createBasicSourceTree(dirs.source);

    await appPage.openTab("手动备份");
    await backupPage.fillPaths(dirs.source, dirs.backup);
    await backupPage.setArchive("zip");
    await backupPage.submit();
    await waitForTask((task) => task.type === "backup" && task.status === "success");
    const zipBackup = latestBackup(dirs.backup);
    expect(zipBackup).not.toBeNull();

    await appPage.openTab("备份还原");
    await restorePage.fill(zipBackup!, path.join(dirs.restore, "zip"));
    await restorePage.submit();
    await expectGlobalMessage("还原任务已创建");
    await waitForTask((task) => task.type === "restore" && task.status === "success");

    await appPage.openTab("手动备份");
    await backupPage.fillPaths(dirs.source, path.join(dirs.backup, "enc"));
    await backupPage.setArchive("zip");
    await backupPage.setEncrypt("secret");
    await backupPage.submit();
    await waitForTask((task) => task.type === "backup" && task.status === "success");
    const encBackup = latestBackup(path.join(dirs.backup, "enc"));
    expect(encBackup).not.toBeNull();

    await appPage.openTab("备份还原");
    await restorePage.fill(encBackup!, path.join(dirs.restore, "enc-wrong"), "wrong");
    await restorePage.submit();
    await waitForTask((task) => task.type === "restore" && task.status === "failed");

    await appPage.openTab("备份还原");
    await restorePage.fill(encBackup!, path.join(dirs.restore, "enc-ok"), "secret");
    await restorePage.submit();
    await waitForTask((task) => task.type === "restore" && task.status === "success");
  });

  it("shows validation and missing backup source failures", async () => {
    const dirs = createTestDirs("restore_errors");
    await appPage.openTab("备份还原");
    await restorePage.submit();
    await expectGlobalMessage("请输入备份文件或目录路径");

    await restorePage.fill(path.join(dirs.backup, "missing.zip"), dirs.restore);
    await restorePage.submit();
    await waitForTask((task) => task.type === "restore" && task.status === "failed");
    expect(fs.existsSync(path.join(dirs.restore, "missing.zip"))).toBe(false);
  });
});
