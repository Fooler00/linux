import { authPage } from "../page-objects/AuthPage";
import { appPage } from "../page-objects/AppPage";
import { resetBrowserStorage } from "../helpers/cleanup";
import { runId } from "../helpers/env";

describe("AuthPanel", () => {
  beforeEach(async () => {
    await appPage.logoutIfNeeded();
    await resetBrowserStorage();
    await browser.refresh();
  });

  it("registers, enters the main UI, logs out, and logs in again", async () => {
    const username = `${runId}_auth`;
    const password = "pass_123456";

    await authPage.switchToRegister();
    await authPage.fill(username, password);
    await authPage.submit("注册并进入");
    await expect(await $("button=退出登录")).toBeDisplayed();
    await expect(await $("button=手动备份")).toBeDisplayed();

    await appPage.logoutIfNeeded();
    await expect(await $("button=登录")).toBeDisplayed();

    await authPage.fill(username, password);
    await authPage.submit("登录");
    await expect(await $("button=退出登录")).toBeDisplayed();
  });

  it("shows errors for wrong password and missing required fields", async () => {
    const username = `${runId}_wrong`;
    const password = "pass_abcdef";

    await authPage.switchToRegister();
    await authPage.fill(username, password);
    await authPage.submit("注册并进入");
    await appPage.logoutIfNeeded();

    await authPage.fill(username, "wrong-password");
    await authPage.submit("登录");
    await authPage.expectError("用户名或密码错误");

    await $('input[placeholder="请输入用户名"]').setValue("");
    await $('input[placeholder="请输入密码"]').setValue("");
    await authPage.submit("登录");
    await authPage.expectError("请输入用户名");
  });
});
