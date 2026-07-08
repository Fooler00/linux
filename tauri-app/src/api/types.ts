export type ArchiveType = "none" | "zip" | "tar" | "tar.gz";

export type EncryptAlgo =
  | "aes-256-cbc"
  | "aes-128-cbc"
  | "camellia-256-cbc"
  | "camellia-128-cbc"
  | "des-ede3-cbc"
  | "chacha20";

export type ArchiveTypeValue = ArchiveType | (string & {});

export type EncryptAlgoValue = EncryptAlgo | (string & {});

// 页面在逐步收口过程中先保留 string 兼容，避免旧表单值在过渡阶段把构建卡死。

export interface Task {
  id: number;
  userId: number;
  username: string;
  type: string;
  source: string;
  destination: string;
  status: "running" | "success" | "failed" | string;
  message: string;
  createdAt: string;
}

export interface BackupFilter {
  includePaths?: string[];
  excludePaths?: string[];
  extensions?: string[];
  fileNameContains?: string;
  minSize?: number;
  maxSize?: number;
  modifiedAfter?: string;
  modifiedBefore?: string;
  uid?: number;
  gid?: number;
  owner?: string;
  group?: string;
  preserveMetadata?: boolean;
  includeSpecialFiles?: boolean;
  archiveType?: ArchiveTypeValue;
  encryptAlgo?: EncryptAlgoValue;
  incremental?: boolean;
  incrementalBase?: string;
}

export interface BackupRequest {
  source: string;
  destination: string;
  compress?: boolean;
  encrypt?: boolean;
  password?: string;
  archiveType?: ArchiveTypeValue;
  encryptAlgo?: EncryptAlgoValue;
  preserveMetadata?: boolean;
  includeSpecialFiles?: boolean;
  incremental?: boolean;
  incrementalBase?: string;
  filter?: BackupFilter;
}

export interface RestoreRequest {
  backupPath: string;
  destination: string;
  password?: string;
  encryptAlgo?: EncryptAlgoValue;
}

export interface WatchRequest {
  source: string;
  destination: string;
  intervalSeconds?: number;
  filter?: BackupFilter;
}

export interface ScheduleRequest {
  source: string;
  destination: string;
  intervalSeconds?: number;
  maxBackups?: number;
  maxAgeDays?: number;
  compress?: boolean;
  encrypt?: boolean;
  password?: string;
  filter?: BackupFilter;
}

export interface Schedule {
  id: number;
  source: string;
  destination: string;
  intervalSeconds: number;
  maxBackups: number;
  maxAgeDays: number;
  compress: boolean;
  encrypt: boolean;
  username: string;
}

export interface BackupItem {
  name: string;
  path: string;
  size: number;
  timestamp: number;
}

export interface BackupMetadata {
  source: string;
  backupTime: string;
  mode: string;
  size: number;
  copiedFiles: number;
  userId: number;
  username: string;
  // TODO: /api/metadata 直接读取 metadata.json，历史备份里字段可能缺失或不是当前白名单值。
  archiveType: ArchiveTypeValue;
  encryptAlgo: EncryptAlgoValue;
  preserveMetadata: boolean;
  includeSpecialFiles: boolean;
  incremental: boolean;
  filter: BackupFilter;
}

export interface ApiError {
  error: string;
}
