import path from "node:path";
import { authPage } from "../page-objects/AuthPage";
import { appPage } from "../page-objects/AppPage";
import { schedulePage } from "../page-objects/SchedulePage";
import { createBasicSourceTree, createTestDirs, countBackups } from "../helpers/filesystem";
import { expectGlobalMessage } from "../helpers/assertions";
import { stopAllSchedules, waitForTask } from "../helpers/server";
import { runId } from "../helpers/env";

describe("SchedulePanel", () => {
  before(async () => {
    await appPage.logoutIfNeeded();
    await authPage.switchToRegister();
    await authPage.fill(`${runId}_schedule`, "pass_123456");
    await authPage.submit("注册并进入");
  });

  afterEach(async () => {
    await stopAllSchedules();
  });

  it("creates scheduled backups, applies retention, and stops a schedule", async () => {
    const dirs = createTestDirs("schedule_flow");
    createBasicSourceTree(dirs.source);
    await appPage.openTab("定时备份");
    await schedulePage.fill(dirs.source, dirs.backup, 1, 2);
    await schedulePage.start();
    await expectGlobalMessage("定时备份已启动");
    await schedulePage.refresh();
    await expect(await $("td=1")).toBeDisplayed();

    await waitForTask((task) => task.type === "scheduled-backup" && task.status === "success", 90000);
    await waitForTask((task) => task.type === "scheduled-backup" && task.status === "success", 90000);
    await browser.pause(2500);
    expect(countBackups(dirs.backup)).toBeLessThanOrEqual(2);

    await schedulePage.stopFirst();
    await expectGlobalMessage("定时备份已停止");
  });

  it("reports invalid interval and missing source failures", async () => {
    const dirs = createTestDirs("schedule_errors");
    createBasicSourceTree(dirs.source);
    await appPage.openTab("定时备份");
    await schedulePage.fill(dirs.source, dirs.backup, -1, 0);
    await schedulePage.start();
    await expectGlobalMessage("定时间隔必须大于 0");

    await schedulePage.fill(path.join(dirs.source, "missing"), dirs.backup, 1, 0);
    await schedulePage.start();
    await expectGlobalMessage("定时备份已启动");
    await waitForTask((task) => task.type === "scheduled-backup" && task.status === "failed", 90000);
  });
});
