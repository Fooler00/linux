import path from "node:path";
import { authPage } from "../page-objects/AuthPage";
import { appPage } from "../page-objects/AppPage";
import { cloudPage } from "../page-objects/CloudPage";
import { createCloudFixture, createTestDirs } from "../helpers/filesystem";
import { expectGlobalMessage, expectVisibleText } from "../helpers/assertions";
import { runId } from "../helpers/env";

describe("CloudPanel", () => {
  before(async () => {
    await appPage.logoutIfNeeded();
    await authPage.switchToRegister();
    await authPage.fill(`${runId}_cloud`, "pass_123456");
    await authPage.submit("注册并进入");
  });

  it("uploads, lists, and deletes cloud files with special names", async () => {
    const dirs = createTestDirs("cloud_flow");
    const specialName = "中文 file;quoted.txt";
    const localPath = createCloudFixture(dirs.source, specialName);
    const remoteDir = `e2e-cloud-${runId}`;
    const remotePath = `${remoteDir}/${specialName}`;

    await appPage.openTab("云存储");
    await cloudPage.setRemoteDir(remoteDir);
    await cloudPage.refresh();
    await expectVisibleText("当前目录暂无文件");

    await cloudPage.fillUpload(localPath, remotePath);
    await cloudPage.upload();
    await expectGlobalMessage("上传成功");
    await expectVisibleText(specialName);

    await cloudPage.deleteFirst();
    await expectGlobalMessage("删除成功");
    await expectVisibleText("当前目录暂无文件");
  });

  it("reports missing local file upload failures", async () => {
    const dirs = createTestDirs("cloud_errors");
    await appPage.openTab("云存储");
    await cloudPage.fillUpload(path.join(dirs.source, "missing.zip"), `missing/${runId}.zip`);
    await cloudPage.upload();
    await expectGlobalMessage("本地文件不存在");
    expect(await $("button=上传到云端")).toBeDisplayed();
  });
});
