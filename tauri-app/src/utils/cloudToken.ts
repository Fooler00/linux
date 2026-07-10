const CLOUD_TOKEN_KEY = "backup-cloud-token";

export function readCloudToken(): string {
  return localStorage.getItem(CLOUD_TOKEN_KEY)?.trim() ?? "";
}

export function saveCloudToken(token: string) {
  const value = token.trim();

  if (value) {
    localStorage.setItem(CLOUD_TOKEN_KEY, value);
  } else {
    localStorage.removeItem(CLOUD_TOKEN_KEY);
  }
}

export function getCloudToken(): string | null {
  const token = readCloudToken();
  return token || null;
}
