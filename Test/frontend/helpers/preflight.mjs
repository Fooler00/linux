import fs from "node:fs";
import net from "node:net";
import path from "node:path";
import { spawnSync } from "node:child_process";

const repoRoot = path.resolve(process.cwd(), "..");
const appBinary = path.join(repoRoot, "tauri-app/src-tauri/target/debug/tauri-app");
const sidecarBinary = path.join(
  repoRoot,
  "tauri-app/src-tauri/bin/backup_server-x86_64-unknown-linux-gnu",
);

function commandExists(command) {
  const result = spawnSync("bash", ["-lc", `command -v ${command}`], {
    stdio: "ignore",
  });
  return result.status === 0;
}

async function isPortFree(port) {
  return await new Promise((resolve) => {
    const server = net.createServer();
    server.once("error", () => resolve(false));
    server.once("listening", () => {
      server.close(() => resolve(true));
    });
    server.listen(port, "127.0.0.1");
  });
}

const missing = [];
if (!fs.existsSync(appBinary)) missing.push(`Tauri app binary: ${appBinary}`);
if (!fs.existsSync(sidecarBinary)) missing.push(`Tauri sidecar binary: ${sidecarBinary}`);
if (!commandExists("tauri-driver")) missing.push("tauri-driver");
if (!commandExists("WebKitWebDriver") && !commandExists("webkit2gtk-driver")) {
  missing.push("WebKitWebDriver/webkit2gtk-driver");
}
if (!(await isPortFree(8080))) missing.push("free TCP port 8080");

if (missing.length) {
  console.error(
    `Frontend E2E preflight failed. Missing or unavailable: ${missing.join(", ")}`
  );
  process.exit(1);
}

console.log("Frontend E2E preflight passed.");
