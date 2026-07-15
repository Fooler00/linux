export async function waitUntil(
  condition: () => Promise<boolean>,
  timeout = 10000,
  message = "condition was not met"
) {
  await browser.waitUntil(condition, {
    timeout,
    interval: 300,
    timeoutMsg: message,
  });
}

export async function waitForText(text: string, timeout = 10000) {
  await waitUntil(
    async () => (await browser.$(`//*[contains(normalize-space(), "${text}")]`)).isDisplayed(),
    timeout,
    `Expected visible text: ${text}`
  );
}

export async function pause(ms: number) {
  await browser.pause(ms);
}
