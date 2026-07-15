export async function expectGlobalMessage(partial: string) {
  const bar = await browser.$(".message-bar");
  await bar.waitForDisplayed({ timeout: 10000 });
  await expect(bar).toHaveText(expect.stringContaining(partial));
}

export async function expectVisibleText(partial: string) {
  const node = await browser.$(`//*[contains(normalize-space(), "${partial}")]`);
  await node.waitForDisplayed({ timeout: 10000 });
  await expect(node).toBeDisplayed();
}

export async function expectNoVisibleText(partial: string) {
  const node = await browser.$(`//*[contains(normalize-space(), "${partial}")]`);
  await expect(await node.isDisplayed().catch(() => false)).toBe(false);
}
