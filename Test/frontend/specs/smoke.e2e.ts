import { expect, $ } from "@wdio/globals";

const uniqueUser = `e2e_${Date.now()}`;

async function byText(text: string) {
  return $(`//*[normalize-space(.)="${text}"]`);
}

describe("AuthPanel smoke", () => {
  it("registers a new user and reaches the backup workspace", async () => {
    await expect(await byText("登录你的账号")).toBeDisplayed();
    await (await byText("注册")).click();
    await $("input[placeholder='请输入用户名']").setValue(uniqueUser);
    await $("input[placeholder='请输入密码']").setValue("e2e-password");
    await (await byText("注册并进入")).click();
    await expect(await byText("手动备份")).toBeDisplayed();
    await expect(await byText("任务列表")).toBeDisplayed();
  });
});
