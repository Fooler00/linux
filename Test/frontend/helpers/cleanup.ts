import { stopAllSchedules, stopWatch } from "./server";

export async function resetBrowserStorage() {
  await browser.execute(() => {
    window.sessionStorage.clear();
    window.localStorage.clear();
  });
}

export async function cleanupBackendState() {
  await stopWatch();
  await stopAllSchedules();
}
