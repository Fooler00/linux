import net from "node:net";
import fs from "node:fs";
import { spawnSync } from "node:child_process";
import { apiBase, appBinary, sidecarBinary } from "./env";

export interface TaskRecord {
  id: number;
  type: string;
  source: string;
  destination: string;
  status: string;
  message: string;
  createdAt: string;
}

function commandExists(command: string) {
  const result = spawnSync("bash", ["-lc", `command -v ${command}`], {
    stdio: "ignore",
  });
  return result.status === 0;
}

async function isPortFree(port: number) {
  return await new Promise<boolean>((resolve) => {
    const server = net.createServer();
    server.once("error", () => resolve(false));
    server.once("listening", () => {
      server.close(() => resolve(true));
    });
    server.listen(port, "127.0.0.1");
  });
}

export async function preflight() {
  const missing: string[] = [];
  if (!fs.existsSync(appBinary)) missing.push(`Tauri app binary: ${appBinary}`);
  if (!fs.existsSync(sidecarBinary)) missing.push(`Tauri sidecar binary: ${sidecarBinary}`);
  if (!commandExists("tauri-driver")) missing.push("tauri-driver");
  if (!commandExists("WebKitWebDriver") && !commandExists("webkit2gtk-driver")) {
    missing.push("WebKitWebDriver/webkit2gtk-driver");
  }
  if (!(await isPortFree(8080))) {
    missing.push("free TCP port 8080");
  }
  if (missing.length) {
    throw new Error(
      `Frontend E2E preflight failed. Missing or unavailable: ${missing.join(", ")}`
    );
  }
}

export async function apiRequest<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`${apiBase}${path}`, init);
  const text = await response.text();
  if (!response.ok) {
    throw new Error(text || `HTTP ${response.status}`);
  }
  return text ? (JSON.parse(text) as T) : ({} as T);
}

export async function getTasks(): Promise<TaskRecord[]> {
  return apiRequest<TaskRecord[]>("/api/tasks");
}

export async function waitForTask(
  predicate: (task: TaskRecord) => boolean,
  timeoutMs = 60000
) {
  const started = Date.now();
  let lastTasks: TaskRecord[] = [];
  while (Date.now() - started < timeoutMs) {
    lastTasks = await getTasks().catch(() => []);
    const found = lastTasks.find(predicate);
    if (found) return found;
    await new Promise((resolve) => setTimeout(resolve, 1000));
  }
  throw new Error(`Timed out waiting for task. Last tasks: ${JSON.stringify(lastTasks)}`);
}

export async function stopAllSchedules() {
  const schedules = await apiRequest<Array<{ id: number }>>("/api/schedules").catch(() => []);
  for (const schedule of schedules) {
    await apiRequest("/api/schedule/stop", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ scheduleId: schedule.id }),
    }).catch(() => undefined);
  }
}

export async function stopWatch() {
  await apiRequest("/api/watch/stop", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: "{}",
  }).catch(() => undefined);
}
