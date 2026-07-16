import { expect, $ } from "@wdio/globals";
import fs from "node:fs";
import path from "node:path";

const fixtureRoot = path.resolve(process.cwd(), "../Test/output/frontend/e2e-fixtures");

async function byText(text: string) {
  return $(`//*[normalize-space(.)="${text}"]`);
}

async function visiblePanel() {
  const panels = await $$("section.panel");
  for (const panel of panels) {
    if (await panel.isDisplayed()) {
      return panel;
    }
  }
  throw new Error("No visible panel found");
}

async function panelText() {
  return (await visiblePanel()).getText();
}

async function expectPanelText(text: string) {
  await browser.waitUntil(
    async () => (await panelText()).includes(text),
    {
      timeout: 5000,
      timeoutMsg: `Expected visible panel to contain text: ${text}`,
    }
  );
}

async function panelInputByPlaceholder(placeholder: string) {
  const panel = await visiblePanel();
  const input = await panel.$(`input[placeholder='${placeholder}']`);
  await expect(input).toBeDisplayed();
  return input;
}

async function panelButtonByText(text: string) {
  const panel = await visiblePanel();
  const buttons = await panel.$$("button");
  for (const button of buttons) {
    if ((await button.isDisplayed()) && (await button.getText()).trim() === text) {
      return button;
    }
  }
  throw new Error(`No visible button found in active panel: ${text}`);
}

async function clickPanelButton(text: string) {
  const button = await panelButtonByText(text);
  await button.scrollIntoView();
  await browser.execute((element: HTMLElement) => element.click(), button);
}

async function openTab(tab: string, heading: string) {
  await (await byText(tab)).click();
  await expectPanelText(heading);
}

describe("Backup Manager navigation and UI smoke", () => {
  const username = `e2e_flow_${Date.now()}`;
  const runDir = path.join(fixtureRoot, username);
  const emptyBackupDir = path.join(runDir, "empty-backups");

  before(async () => {
    fs.mkdirSync(emptyBackupDir, { recursive: true });
    await (await byText("注册")).click();
    await $("input[placeholder='请输入用户名']").setValue(username);
    await $("input[placeholder='请输入密码']").setValue("e2e-password");
    await (await byText("注册并进入")).click();
    await expect(await byText("手动备份")).toBeDisplayed();
  });

  it("BackupPanelSmoke_DisplaysBackupForm", async () => {
    await openTab("手动备份", "手动备份");
    await expectPanelText("源路径（文件或目录）");
    await expectPanelText("备份目标目录");
    await expectPanelText("启用压缩");
    await expectPanelText("开始备份");
  });

  it("RestorePanelSmoke_DisplaysRestoreForm", async () => {
    await openTab("备份还原", "还原备份");
    await expectPanelText("备份文件/目录路径");
    await expectPanelText("还原目标目录");
    await expectPanelText("解密密码（加密备份需要）");
    await expectPanelText("开始还原");
  });

  it("WatchPanelSmoke_DisplaysWatchControls", async () => {
    await openTab("实时监听", "实时监听备份");
    await expect(await byText("启动监听")).toBeDisplayed();
    await expect(await byText("停止监听")).toBeDisplayed();
  });

  it("SchedulePanelSmoke_DisplaysScheduleControls", async () => {
    await openTab("定时备份", "定时备份");
    await expect(await byText("启动定时备份")).toBeDisplayed();
    await expect(await byText("运行中的定时任务")).toBeDisplayed();
  });

  it("BackupManagePanelSmoke_DisplaysManagementControls", async () => {
    await openTab("备份管理", "备份管理与淘汰");
    await expectPanelText("查询备份");
    await expectPanelText("执行淘汰");
    await expectPanelText("备份目标目录");
    await expectPanelText("输入备份目录后可查询备份列表，并查看对应 metadata。");
  });

  it("CloudPanelSmoke_DisplaysCloudControls", async () => {
    await openTab("云存储", "云存储");
    await expectPanelText("云存储 Token（可选，对应服务端 BACKUP_CLOUD_TOKEN）");
    await expectPanelText("保存");
    await expectPanelText("刷新列表");
    await expectPanelText("上传到云端");
  });

  it("TaskListSmoke_DisplaysTaskListState", async () => {
    await openTab("任务列表", "任务列表");
    await expectPanelText("手动刷新");
    await expectPanelText("暂无任务");
  });
});
