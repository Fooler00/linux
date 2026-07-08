import { apiRequestOrThrow } from "./client";
import type {
  BackupItem,
  BackupMetadata,
  BackupRequest,
  RestoreRequest,
  Schedule,
  ScheduleRequest,
  Task,
  WatchRequest,
} from "./types";

export function startBackup(payload: BackupRequest) {
  return apiRequestOrThrow<{ taskId: number }>("POST", "/api/backup", payload);
}

export function startRestore(payload: RestoreRequest) {
  return apiRequestOrThrow<{ taskId: number }>("POST", "/api/restore", payload);
}

export function startWatch(payload: WatchRequest) {
  return apiRequestOrThrow<{ message: string }>("POST", "/api/watch/start", payload);
}

export function stopWatch() {
  return apiRequestOrThrow<{ message: string }>("POST", "/api/watch/stop", {});
}

export function getTasks() {
  return apiRequestOrThrow<Task[]>("GET", "/api/tasks");
}

export function startSchedule(payload: ScheduleRequest) {
  return apiRequestOrThrow<{ scheduleId: number; message: string }>(
    "POST",
    "/api/schedule/start",
    payload
  );
}

export function stopSchedule(scheduleId: number) {
  return apiRequestOrThrow<{ success: boolean; message: string }>(
    "POST",
    "/api/schedule/stop",
    { scheduleId }
  );
}

export function getSchedules() {
  return apiRequestOrThrow<Schedule[]>("GET", "/api/schedules");
}

export function pruneBackups(
  destination: string,
  maxBackups: number,
  maxAgeDays: number
) {
  return apiRequestOrThrow<{ removed: number; message: string }>(
    "POST",
    "/api/prune",
    { destination, maxBackups, maxAgeDays }
  );
}

export function listBackups(destination: string) {
  return apiRequestOrThrow<BackupItem[]>("GET", "/api/backups", undefined, {
    destination,
  });
}

export function getMetadata(path: string) {
  return apiRequestOrThrow<BackupMetadata>("GET", "/api/metadata", undefined, {
    path,
  });
}
