import { invoke } from "@tauri-apps/api/core";
import { apiRequestOrThrow } from "./client";
import type { CloudFile, CloudMessageResponse } from "./types";
import { getCloudToken } from "../utils/cloudToken";

function getApiErrorMessage(error: unknown): string {
  const message = error instanceof Error ? error.message : String(error ?? "");

  if (!message) {
    return "请求失败";
  }

  try {
    const parsed = JSON.parse(message) as { error?: string };
    return parsed.error || message;
  } catch {
    return message;
  }
}

export function listCloudFiles(remoteDir: string) {
  return apiRequestOrThrow<CloudFile[]>("GET", "/api/cloud/list", undefined, {
    dir: remoteDir,
  });
}

export function deleteCloudFile(remotePath: string) {
  return apiRequestOrThrow<CloudMessageResponse>("POST", "/api/cloud/delete", {
    remotePath,
  });
}

export async function uploadCloudFile(localPath: string, remotePath: string) {
  try {
    const raw = await invoke<string>("cloud_upload", {
      localPath,
      remotePath,
      cloudToken: getCloudToken(),
    });

    try {
      return JSON.parse(raw) as CloudMessageResponse;
    } catch {
      return { message: raw } satisfies CloudMessageResponse;
    }
  } catch (error) {
    throw new Error(getApiErrorMessage(error));
  }
}

export async function downloadCloudFile(remotePath: string, localPath: string) {
  try {
    return await invoke<string>("cloud_download", {
      remotePath,
      localPath,
      cloudToken: getCloudToken(),
    });
  } catch (error) {
    throw new Error(getApiErrorMessage(error));
  }
}
