import { getTasks } from "../api/backup";
import type { Task } from "../api/types";

const POLL_INTERVAL_MS = 1500;
const DEFAULT_TIMEOUT_MS = 10 * 60 * 1000;

function delay(ms: number) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

/** 轮询 /api/tasks，直到指定任务变为 success/failed 或超时。 */
export async function watchTaskUntilDone(
  taskId: number,
  options?: { intervalMs?: number; timeoutMs?: number }
): Promise<Task> {
  const intervalMs = options?.intervalMs ?? POLL_INTERVAL_MS;
  const timeoutMs = options?.timeoutMs ?? DEFAULT_TIMEOUT_MS;
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    const tasks = await getTasks();
    const task = tasks.find((item) => item.id === taskId);

    if (task && (task.status === "success" || task.status === "failed")) {
      return task;
    }

    await delay(intervalMs);
  }

  throw new Error(`任务 ${taskId} 等待超时`);
}
