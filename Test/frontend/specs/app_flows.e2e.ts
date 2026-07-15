import { expect, $ } from "@wdio/globals";

async function byText(text: string) {
  return $(`//*[normalize-space(.)="${text}"]`);
}

async function openTab(tab: string, heading: string) {
  await (await byText(tab)).click();
  await expect(await byText(heading)).toBeDisplayed();
}

describe("Backup Manager main flows", () => {
  before(async () => {
    const username = `e2e_flow_${Date.now()}`;
    await (await byText("注册")).click();
    await $("input[placeholder='请输入用户名']").setValue(username);
    await $("input[placeholder='请输入密码']").setValue("e2e-password");
    await (await byText("注册并进入")).click();
    await expect(await byText("手动备份")).toBeDisplayed();
  });

  it("covers BackupPanel navigation and validation", async () => {
    await openTab("手动备份", "手动备份");
    await expect($("input[placeholder='/path/to/source']")).toBeDisplayed();
    await expect($("input[placeholder='/path/to/backup']")).toBeDisplayed();
    await (await byText("开始备份")).click();
    await expect(await byText("请检查备份表单")).toBeDisplayed();
  });

  it("covers RestorePanel navigation and validation", async () => {
    await openTab("备份还原", "还原备份");
    await expect($("input[placeholder='/path/to/backup_xxx']")).toBeDisplayed();
    await expect($("input[placeholder='/path/to/restore']")).toBeDisplayed();
    await (await byText("开始还原")).click();
    await expect(await byText("请检查还原表单")).toBeDisplayed();
  });

  it("covers WatchPanel controls", async () => {
    await openTab("实时监听", "实时监听备份");
    await expect(await byText("启动监听")).toBeDisplayed();
    await expect(await byText("停止监听")).toBeDisplayed();
  });

  it("covers SchedulePanel controls", async () => {
    await openTab("定时备份", "定时备份");
    await expect(await byText("启动定时备份")).toBeDisplayed();
    await expect(await byText("运行中的定时任务")).toBeDisplayed();
  });

  it("covers BackupManagePanel controls", async () => {
    await openTab("备份管理", "备份管理与淘汰");
    await expect(await byText("查询备份")).toBeDisplayed();
    await expect(await byText("暂无备份记录")).toBeDisplayed();
  });

  it("covers CloudPanel controls", async () => {
    await openTab("云存储", "云存储");
    await expect(await byText("保存 Token")).toBeDisplayed();
    await expect(await byText("上传到云端")).toBeDisplayed();
  });

  it("covers TaskList controls", async () => {
    await openTab("任务列表", "任务列表");
    await expect(await byText("刷新")).toBeDisplayed();
  });
});
