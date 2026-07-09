import { invoke } from "@tauri-apps/api/core";
import type { ApiError } from "./types";
import { getSessionToken } from "../utils/session";

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
    authToken: getSessionToken(),
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
    throw new Error(getApiErrorMessage(error));
  }
}

function getApiErrorMessage(error: unknown): string {
  const message = error instanceof Error ? error.message : String(error ?? "");

  if (!message) {
    return "请求失败";
  }

  try {
    const parsed = JSON.parse(message) as Partial<ApiError>;
    // Rust 层会把后端非 2xx 响应体原样作为错误抛出，这里统一提取后端 { error } 文案。
    return parsed.error || message;
  } catch {
    return message;
  }
}
