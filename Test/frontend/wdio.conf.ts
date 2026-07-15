import path from "node:path";
import { createRequire } from "node:module";
import {
  appBinary,
  logsDir,
  outputRoot,
  repoRoot,
  runtimeRoot,
  screenshotsDir,
} from "./helpers/env";
import { ensureFrontendOutput } from "./helpers/filesystem";
import { preflight, stopAllSchedules, stopWatch } from "./helpers/server";

const require = createRequire(path.join(repoRoot, "Test/frontend/wdio.conf.ts"));
const LocalTauriService = require(path.join(
  repoRoot,
  "tauri-app/node_modules/@wdio/tauri-service/dist/cjs/index.js",
));

const tauriServiceOptions = {
  appBinaryPath: appBinary,
  driverProvider: "external",
  autoInstallTauriDriver: false,
  autoDownloadEdgeDriver: false,
  nativeDriverPath:
    process.env.WEBKIT_WEBDRIVER_PATH ?? "/usr/bin/WebKitWebDriver",
  tauriDriverPort: Number(process.env.TAURI_DRIVER_PORT ?? 4444),
  startTimeout: 60000,
  commandTimeout: 30000,
  logLevel: "info",
  logDir: logsDir,
  captureBackendLogs: false,
  captureFrontendLogs: false,
};

let tauriLauncherInstance: Record<string, unknown> | undefined;
const localTauriLauncher = {
  async onPrepare(config: unknown, capabilities: unknown) {
    tauriLauncherInstance = new LocalTauriService.launcher(
      tauriServiceOptions,
      capabilities,
      config,
    );
    return callServiceHook(tauriLauncherInstance, "onPrepare", config, capabilities);
  },
  async onWorkerStart(...args: unknown[]) {
    return callServiceHook(tauriLauncherInstance, "onWorkerStart", ...args);
  },
  async onWorkerEnd(...args: unknown[]) {
    return callServiceHook(tauriLauncherInstance, "onWorkerEnd", ...args);
  },
  async onComplete(...args: unknown[]) {
    return callServiceHook(tauriLauncherInstance, "onComplete", ...args);
  },
};

function callServiceHook(
  target: Record<string, unknown> | undefined,
  name: string,
  ...args: unknown[]
) {
  const hook = target?.[name];
  if (typeof hook !== "function") return undefined;
  return hook.apply(target, args);
}

export const config = {
  runner: "local",
  specs: [path.join(repoRoot, "Test/frontend/specs/**/*.e2e.ts")],
  maxInstances: 1,
  capabilities: [
    {
      browserName: "tauri",
      "tauri:options": {
        application: appBinary,
      },
    },
  ],
  services: [
    [localTauriLauncher, {}],
    [LocalTauriService.default, tauriServiceOptions],
  ],
  logLevel: "info",
  outputDir: logsDir,
  bail: 0,
  waitforTimeout: 10000,
  connectionRetryTimeout: 90000,
  connectionRetryCount: 2,
  framework: "mocha",
  reporters: [["spec", { showPreface: false }]],
  mochaOpts: {
    ui: "bdd",
    timeout: 120000,
  },
  onPrepare: async () => {
    ensureFrontendOutput();
    process.env.XDG_DATA_HOME = runtimeRoot;
    await preflight();
  },
  before: async () => {
    await browser.setTimeout({ script: 30000, pageLoad: 30000, implicit: 0 });
  },
  afterTest: async (test, _context, result) => {
    if (result.passed) return;
    const safeName = `${Date.now()}_${test.parent}_${test.title}`
      .replace(/[^\w.-]+/g, "_")
      .slice(0, 180);
    await browser.saveScreenshot(path.join(screenshotsDir, `${safeName}.png`));
  },
  after: async () => {
    await stopWatch();
    await stopAllSchedules();
  },
} satisfies WebdriverIO.Config;

console.log(`Frontend E2E output root: ${outputRoot}`);
