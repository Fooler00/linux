import path from "node:path";
import { fileURLToPath } from "node:url";
import type { Options } from "@wdio/types";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, "../..");
const tauriRoot = path.join(repoRoot, "tauri-app");
const appBinary =
  process.env.E2E_TAURI_APP_BIN ??
  path.join(tauriRoot, "src-tauri", "target", "debug", "tauri-app");

export const config: Options.Testrunner = {
  runner: "local",
  specs: [path.join(__dirname, "specs", "*.e2e.ts")],
  maxInstances: 1,
  logLevel: "info",
  outputDir: path.join(repoRoot, "Test", "output", "frontend", "wdio-logs"),
  framework: "mocha",
  reporters: ["spec"],
  mochaOpts: {
    timeout: 120000,
  },
  services: [
    [
      "@wdio/tauri-service",
      {
        appBinaryPath: appBinary,
        driverProvider: "official",
        tauriDriverPort: 4444,
        startTimeout: 90000,
        autoInstallTauriDriver: false,
        captureBackendLogs: false,
        captureFrontendLogs: false,
      },
    ],
  ],
  capabilities: [
    {
      browserName: "tauri",
      "tauri:options": {
        application: appBinary,
        driverProvider: "official",
      },
    } as WebdriverIO.Capabilities,
  ],
  before: async () => {
    await browser.setTimeout({ implicit: 3000, pageLoad: 30000, script: 30000 });
  },
  afterTest: async (test, _context, result) => {
    if (result.passed) {
      return;
    }
    const screenshotDir = path.join(repoRoot, "Test", "output", "frontend", "screenshots");
    await browser.saveScreenshot(path.join(screenshotDir, `${Date.now()}-${test.title.replace(/\W+/g, "_")}.png`));
  },
};
