import type { ArchiveType, EncryptAlgo } from "../api/types";
import { ARCHIVE_TYPES, ENCRYPT_ALGOS } from "./options";

export interface ValidationResult {
  valid: boolean;
  message?: string;
}

export interface BackupValidationInput {
  source?: string;
  sources?: string[];
  destination: string;
  encrypt: boolean;
  password?: string;
  archiveType?: string;
  encryptAlgo?: string;
  incremental?: boolean;
}

export interface RestoreValidationInput {
  backupPath: string;
  destination: string;
  encryptAlgo?: string;
}

export function isSupportedArchiveType(value: string): value is ArchiveType {
  return ARCHIVE_TYPES.includes(value as ArchiveType);
}

export function isSupportedEncryptAlgo(value: string): value is EncryptAlgo {
  return ENCRYPT_ALGOS.includes(value as EncryptAlgo);
}

export function validateBackupForm(input: BackupValidationInput): ValidationResult {
  const destination = input.destination.trim();
  const sources = (input.sources ?? []).map((item) => item.trim()).filter(Boolean);
  const source = input.source?.trim() ?? "";
  const multiSourceMode = sources.length > 0;

  if (!multiSourceMode && !source) {
    return invalid("请输入源路径（文件或目录），或选择多个文件");
  }

  if (!destination) {
    return invalid("请输入备份目标目录");
  }

  if (multiSourceMode) {
    if (input.incremental) {
      return invalid("多文件备份不支持增量模式");
    }

    const uniqueSources = new Set(sources);
    if (uniqueSources.size !== sources.length) {
      return invalid("多文件列表中存在重复路径");
    }

    if (sources.some((item) => item === destination)) {
      return invalid("源文件路径不能与备份目标目录相同");
    }
  } else {
    if (source === destination) {
      return invalid("源路径和备份目标目录不建议相同，请重新选择");
    }
  }

  if (!input.archiveType || !isSupportedArchiveType(input.archiveType)) {
    return invalid("归档格式不受支持，请选择 none、zip、tar 或 tar.gz");
  }

  if (!input.encryptAlgo || !isSupportedEncryptAlgo(input.encryptAlgo)) {
    return invalid("加密算法不受支持，请重新选择");
  }

  // 开启加密时后端会用 password 调 openssl；前端先拦截空密码，避免生成不可还原的演示任务。
  if (input.encrypt && !input.password?.trim()) {
    return invalid("启用加密时请输入密码");
  }

  return { valid: true };
}

export function validateRestoreForm(input: RestoreValidationInput): ValidationResult {
  if (!input.backupPath.trim()) {
    return invalid("请输入备份文件或目录路径");
  }

  if (!input.destination.trim()) {
    return invalid("请输入还原目标目录");
  }

  if (input.encryptAlgo && !isSupportedEncryptAlgo(input.encryptAlgo)) {
    return invalid("加密算法不受支持，请重新选择");
  }

  return { valid: true };
}

export function validateBackupManageDestination(destination: string): ValidationResult {
  // 备份管理页所有真实接口都依赖目录参数，先统一拦截空值，避免无意义请求和误导性报错。
  if (!destination.trim()) {
    return invalid("请输入备份目标目录");
  }

  return { valid: true };
}

function invalid(message: string): ValidationResult {
  return { valid: false, message };
}
