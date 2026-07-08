import type { ArchiveType, EncryptAlgo } from "../api/types";

export const ARCHIVE_TYPE_OPTIONS = [
  { label: "不归档", value: "none" },
  { label: "zip", value: "zip" },
  { label: "tar", value: "tar" },
  { label: "tar.gz", value: "tar.gz" },
] as const satisfies ReadonlyArray<{ label: string; value: ArchiveType }>;

export const ENCRYPT_ALGO_OPTIONS = [
  { label: "AES-256-CBC", value: "aes-256-cbc" },
  { label: "AES-128-CBC", value: "aes-128-cbc" },
  { label: "Camellia-256-CBC", value: "camellia-256-cbc" },
  { label: "Camellia-128-CBC", value: "camellia-128-cbc" },
  { label: "DES-EDE3-CBC", value: "des-ede3-cbc" },
  { label: "ChaCha20", value: "chacha20" },
] as const satisfies ReadonlyArray<{ label: string; value: EncryptAlgo }>;

export const ARCHIVE_TYPES = ARCHIVE_TYPE_OPTIONS.map((item) => item.value);

export const ENCRYPT_ALGOS = ENCRYPT_ALGO_OPTIONS.map((item) => item.value);
