import type { AuthUser } from "../api/types";

const SESSION_KEY = "backup-session";

export function readSession(): AuthUser | null {
  const raw = sessionStorage.getItem(SESSION_KEY);

  if (!raw) {
    return null;
  }

  try {
    const parsed = JSON.parse(raw) as Partial<AuthUser>;

    if (
      typeof parsed.userId === "number" &&
      typeof parsed.username === "string" &&
      typeof parsed.token === "string" &&
      parsed.username &&
      parsed.token
    ) {
      return {
        userId: parsed.userId,
        username: parsed.username,
        token: parsed.token,
      };
    }
  } catch {
    // Ignore malformed session payloads and treat them as logged out.
  }

  sessionStorage.removeItem(SESSION_KEY);
  return null;
}

export function saveSession(user: AuthUser) {
  sessionStorage.setItem(SESSION_KEY, JSON.stringify(user));
}

export function clearSession() {
  sessionStorage.removeItem(SESSION_KEY);
}

export function getSessionToken(): string | null {
  return readSession()?.token ?? null;
}
