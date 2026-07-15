import path from "node:path";
import { authPage } from "../page-objects/AuthPage";
import { appPage } from "../page-objects/AppPage";
import { backupPage } from "../page-objects/BackupPage";
import { managePage } from "../page-objects/ManagePage";
import { createBasicSourceTree, createTestDirs } from "../helpers/filesystem";
import { expectGlobalMessage, expectVisibleText } from "../helpers/assertions";
import { waitForTask } from "../helpers/server";
import { runId } from "../helpers/env";

describe("BackupManagePanel", () => {
  before(async () => {
    await appPage.logoutIfNeeded();
    await authPage.switchToRegister();
    await authPage.fill(`${runId}_manage`, "pass_123456");
    await authPage.submit("注册并进入");
  });

  it("queries backups, displays metadata, prunes, and handles empty/invalid paths", async () => {
    const dirs = createTestDirs("manage_flow");
    createBasicSourceTree(dirs.source);

    for (const suffix of ["one", "two", "three"]) {
      await appPage.openTab("手动备份");
      await backupPage.fillPaths(dirs.source, path.join(dirs.backup, suffix));
      await backupPage.setArchive("none");
      await backupPage.submit();
      await waitForTask((task) => task.type === "backup" && task.status === "success");
    }

    await appPage.openTab("备份管理");
    await managePage.setDestination(path.join(dirs.backup, "one"));
    await managePage.query();
    await expectGlobalMessage("找到 1 个备份");
    await managePage.viewMetadata();
    await expectVisibleText("元数据");

    await managePage.setDestination(path.join(dirs.backup, "two"));
    await managePage.setMaxBackups(0);
    await managePage.prune();
    await expectGlobalMessage("已淘汰");

    await managePage.setDestination(path.join(dirs.backup, "empty"));
    await managePage.query();
    await expectGlobalMessage("找到 0 个备份");
    await expectVisibleText("暂无备份记录");

    await managePage.setDestination("");
    await managePage.query();
    await expectGlobalMessage("请输入备份目标目录");
  });
});
