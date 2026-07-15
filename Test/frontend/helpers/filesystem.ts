import fs from "node:fs";
import path from "node:path";
import {
  backupRoot,
  cloudRoot,
  downloadsRoot,
  logsDir,
  outputRoot,
  restoreRoot,
  runtimeRoot,
  screenshotsDir,
  sourceRoot,
  tmpRoot,
} from "./env";

export interface TestDirs {
  name: string;
  source: string;
  backup: string;
  restore: string;
  cloud: string;
  downloads: string;
  tmp: string;
}

export function ensureDir(dir: string) {
  fs.mkdirSync(dir, { recursive: true });
}

export function ensureFrontendOutput() {
  [
    outputRoot,
    runtimeRoot,
    logsDir,
    screenshotsDir,
    tmpRoot,
    sourceRoot,
    backupRoot,
    restoreRoot,
    cloudRoot,
    downloadsRoot,
  ].forEach(ensureDir);
}

export function createTestDirs(name: string): TestDirs {
  const dirs = {
    name,
    source: path.join(sourceRoot, name),
    backup: path.join(backupRoot, name),
    restore: path.join(restoreRoot, name),
    cloud: path.join(cloudRoot, name),
    downloads: path.join(downloadsRoot, name),
    tmp: path.join(tmpRoot, name),
  };
  Object.values(dirs)
    .filter((value) => value !== name)
    .forEach((dir) => {
      fs.rmSync(dir, { recursive: true, force: true });
      ensureDir(dir);
    });
  return dirs;
}

export function writeText(filePath: string, content: string) {
  ensureDir(path.dirname(filePath));
  fs.writeFileSync(filePath, content);
}

export function createBasicSourceTree(root: string) {
  ensureDir(root);
  writeText(path.join(root, "alpha.txt"), "alpha\n");
  writeText(path.join(root, "nested/beta.txt"), "beta\n");
  writeText(path.join(root, "app.log"), "log should be filtered when .log is used\n");
}

export function createCloudFixture(root: string, fileName = "cloud file.txt") {
  ensureDir(root);
  const filePath = path.join(root, fileName);
  writeText(filePath, `cloud fixture ${fileName}\n`);
  return filePath;
}

export function latestBackup(destination: string): string | null {
  if (!fs.existsSync(destination)) return null;
  const entries = fs
    .readdirSync(destination)
    .filter((name) => name.startsWith("backup_"))
    .sort();
  if (!entries.length) return null;
  return path.join(destination, entries[entries.length - 1]);
}

export function countBackups(destination: string) {
  if (!fs.existsSync(destination)) return 0;
  return fs.readdirSync(destination).filter((name) => name.startsWith("backup_")).length;
}
