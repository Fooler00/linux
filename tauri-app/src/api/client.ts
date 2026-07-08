import { invoke } from "@tauri-apps/api/core";

export async function apiRequest<T>(
  method: "GET" | "POST",
  path: string,
  body?: unknown,
  query?: Record<string, string>
): Promise<T> {
  const raw = await invoke<string>("api_request", {
    method,
    path,
    body: body !== undefined ? JSON.stringify(body) : null,
    query: query ?? null,
  });

  try {
    return JSON.parse(raw) as T;
  } catch {
    return raw as T;
  }
}

export async function apiRequestOrThrow<T>(
  method: "GET" | "POST",
  path: string,
  body?: unknown,
  query?: Record<string, string>
): Promise<T> {
  try {
    return await apiRequest<T>(method, path, body, query);
  } catch (error) {
    const message = String(error);
    try {
      const parsed = JSON.parse(message) as { error?: string };
      throw new Error(parsed.error ?? message);
    } catch {
      throw new Error(message || "请求失败");
    }
  }
}
