import path from "node:path";

export const repoRoot = process.cwd().endsWith("tauri-app")
  ? path.resolve(process.cwd(), "..")
  : process.cwd();
export const tauriRoot = path.join(repoRoot, "tauri-app");
export const outputRoot = path.join(repoRoot, "Test/output/frontend");
export const runtimeRoot = path.join(outputRoot, "runtime");
export const logsDir = path.join(outputRoot, "logs");
export const screenshotsDir = path.join(outputRoot, "screenshots");
export const tmpRoot = path.join(outputRoot, "tmp");
export const sourceRoot = path.join(outputRoot, "source");
export const backupRoot = path.join(outputRoot, "backup");
export const restoreRoot = path.join(outputRoot, "restore");
export const cloudRoot = path.join(outputRoot, "cloud");
export const downloadsRoot = path.join(outputRoot, "downloads");

export const apiBase = process.env.BACKUP_E2E_API_BASE ?? "http://127.0.0.1:8080";
export const appBinary =
  process.env.TAURI_APP_BINARY ??
  path.join(tauriRoot, "src-tauri/target/debug/tauri-app");
export const sidecarBinary = path.join(
  tauriRoot,
  "src-tauri/bin/backup_server-x86_64-unknown-linux-gnu"
);

export const runId =
  process.env.BACKUP_E2E_RUN_ID ??
  `e2e_${new Date().toISOString().replace(/[-:.TZ]/g, "").slice(0, 14)}_${process.pid}`;

export function specPath(kind: string, name: string) {
  return path.join(outputRoot, kind, `${runId}_${name}`);
}
