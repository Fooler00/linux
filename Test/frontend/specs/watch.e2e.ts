import path from "node:path";
import fs from "node:fs";
import { authPage } from "../page-objects/AuthPage";
import { appPage } from "../page-objects/AppPage";
import { watchPage } from "../page-objects/WatchPage";
import { createBasicSourceTree, createTestDirs } from "../helpers/filesystem";
import { expectGlobalMessage } from "../helpers/assertions";
import { waitForTask, stopWatch } from "../helpers/server";
import { runId } from "../helpers/env";

describe("WatchPanel", () => {
  before(async () => {
    await appPage.logoutIfNeeded();
    await authPage.switchToRegister();
    await authPage.fill(`${runId}_watch`, "pass_123456");
    await authPage.submit("注册并进入");
  });

  afterEach(async () => {
    await stopWatch();
  });

  it("starts, records changes, prevents duplicate start, and stops", async () => {
    const dirs = createTestDirs("watch_flow");
    createBasicSourceTree(dirs.source);

    await appPage.openTab("实时监听");
    await watchPage.fill(dirs.source, dirs.backup, 1);
    await watchPage.start();
    await expectGlobalMessage("实时备份已启动");
    expect(await watchPage.startButtonDisabled()).toBe(true);

    fs.writeFileSync(path.join(dirs.source, "created.txt"), "created\n");
    await waitForTask((task) => task.type === "realtime-backup" && task.status === "success", 90000);

    fs.writeFileSync(path.join(dirs.source, "created.txt"), "modified\n");
    await waitForTask((task) => task.type === "realtime-backup" && task.status === "success", 90000);

    fs.rmSync(path.join(dirs.source, "created.txt"), { force: true });
    await waitForTask((task) => task.type === "realtime-backup" && task.status === "success", 90000);

    await watchPage.stop();
    await expectGlobalMessage("实时备份已停止");
    expect(await watchPage.startButtonDisabled()).toBe(false);
  });

  it("shows validation failure for missing inputs and backend failure for missing source", async () => {
    const dirs = createTestDirs("watch_errors");
    await appPage.openTab("实时监听");
    await watchPage.start();
    await expectGlobalMessage("请输入监听目录和备份目标目录");

    await watchPage.fill(path.join(dirs.source, "missing"), dirs.backup, 1);
    await watchPage.start();
    await expectGlobalMessage("实时备份已启动");
    fs.mkdirSync(dirs.tmp, { recursive: true });
    await waitForTask((task) => task.type === "realtime-backup" && task.status === "failed", 90000).catch(
      async () => {
        await stopWatch();
        throw new Error("Missing source watch did not create a failed realtime task");
      }
    );
  });
});
