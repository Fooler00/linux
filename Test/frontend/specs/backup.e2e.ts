import path from "node:path";
import fs from "node:fs";
import { authPage } from "../page-objects/AuthPage";
import { appPage } from "../page-objects/AppPage";
import { backupPage } from "../page-objects/BackupPage";
import { createBasicSourceTree, createTestDirs, latestBackup } from "../helpers/filesystem";
import { expectGlobalMessage } from "../helpers/assertions";
import { waitForTask } from "../helpers/server";
import { runId } from "../helpers/env";

async function login() {
  await appPage.logoutIfNeeded();
  await authPage.switchToRegister();
  await authPage.fill(`${runId}_backup`, "pass_123456");
  await authPage.submit("注册并进入");
}

describe("BackupPanel", () => {
  before(async () => {
    await login();
  });

  it("creates normal, zip, tar, tar.gz, encrypted, and filtered backups", async () => {
    const dirs = createTestDirs("backup_matrix");
    createBasicSourceTree(dirs.source);

    for (const archiveType of ["none", "zip", "tar", "tar.gz"]) {
      await appPage.openTab("手动备份");
      await backupPage.fillPaths(dirs.source, path.join(dirs.backup, archiveType.replace(".", "_")));
      await backupPage.setArchive(archiveType);
      await backupPage.submit();
      await expectGlobalMessage("备份任务已创建");
      await waitForTask(
        (task) => task.type === "backup" && task.source.includes(dirs.source) && task.status === "success"
      );
      expect(latestBackup(path.join(dirs.backup, archiveType.replace(".", "_")))).not.toBeNull();
    }

    await appPage.openTab("手动备份");
    await backupPage.fillPaths(dirs.source, path.join(dirs.backup, "encrypted"));
    await backupPage.setArchive("zip");
    await backupPage.setEncrypt("secret");
    await backupPage.submit();
    await waitForTask((task) => task.type === "backup" && task.status === "success");
    expect(latestBackup(path.join(dirs.backup, "encrypted"))?.endsWith(".enc")).toBe(true);

    await appPage.openTab("手动备份");
    await backupPage.fillPaths(dirs.source, path.join(dirs.backup, "filtered"));
    await backupPage.setArchive("none");
    await backupPage.openAdvancedFilter();
    await backupPage.setExtensions("txt");
    await backupPage.setExcludePaths(".log");
    await backupPage.submit();
    await waitForTask((task) => task.type === "backup" && task.status === "success");
    const filteredBackup = latestBackup(path.join(dirs.backup, "filtered"));
    expect(filteredBackup).not.toBeNull();
    expect(fs.existsSync(path.join(filteredBackup!, "app.log"))).toBe(false);
  });

  it("shows validation and failed task states", async () => {
    const dirs = createTestDirs("backup_errors");
    await appPage.openTab("手动备份");
    await backupPage.submit();
    await expectGlobalMessage("请输入源路径");

    await backupPage.fillPaths(path.join(dirs.source, "missing"), dirs.backup);
    await backupPage.submit();
    await expectGlobalMessage("备份任务已创建");
    await waitForTask((task) => task.type === "backup" && task.status === "failed");
  });

  it("documents the existing *.log glob filter defect through UI behavior", async () => {
    const dirs = createTestDirs("backup_glob_defect");
    createBasicSourceTree(dirs.source);
    await appPage.openTab("手动备份");
    await backupPage.fillPaths(dirs.source, dirs.backup);
    await backupPage.setArchive("none");
    await backupPage.openAdvancedFilter();
    await backupPage.setExcludePaths("*.log");
    await backupPage.submit();
    await waitForTask((task) => task.type === "backup" && task.status === "success");
    const backup = latestBackup(dirs.backup);
    expect(backup).not.toBeNull();
    expect(fs.existsSync(path.join(backup!, "app.log"))).toBe(true);
  });
});
