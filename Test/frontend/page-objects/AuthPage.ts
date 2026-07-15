export class AuthPage {
  async switchToRegister() {
    const button = await $("button=注册");
    await button.waitForClickable({ timeout: 10000 });
    await button.click();
  }

  async switchToLogin() {
    const button = await $("button=登录");
    await button.waitForClickable({ timeout: 10000 });
    await button.click();
  }

  async fill(username: string, password: string) {
    await $('input[placeholder="请输入用户名"]').setValue(username);
    await $('input[placeholder="请输入密码"]').setValue(password);
  }

  async submit(label: "登录" | "注册并进入" = "登录") {
    const button = await $(`button=${label}`);
    await button.waitForClickable({ timeout: 10000 });
    await button.click();
  }

  async expectError(partial: string) {
    const error = await $(".auth-error");
    await error.waitForDisplayed({ timeout: 10000 });
    await expect(error).toHaveText(expect.stringContaining(partial));
  }
}

export const authPage = new AuthPage();
