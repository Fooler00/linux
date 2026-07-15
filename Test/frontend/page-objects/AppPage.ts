export class AppPage {
  async logoutIfNeeded() {
    const logout = await $("button=退出登录");
    if (await logout.isDisplayed().catch(() => false)) {
      await logout.click();
    }
  }

  async openTab(label: string) {
    const tab = await $(`button=${label}`);
    await tab.waitForClickable({ timeout: 10000 });
    await tab.click();
  }

  async section(title: string) {
    const node = await browser.$(
      `//h2[normalize-space()="${title}"]/ancestor::section[contains(@class,"panel")][1]`
    );
    await node.waitForDisplayed({ timeout: 10000 });
    return node;
  }

  async messageText() {
    const bar = await $(".message-bar");
    await bar.waitForDisplayed({ timeout: 10000 });
    return bar.getText();
  }
}

export const appPage = new AppPage();
