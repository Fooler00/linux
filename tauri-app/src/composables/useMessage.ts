import { reactive } from "vue";

export type MessageType = "info" | "success" | "error";

export interface ToastItem {
  id: number;
  text: string;
  type: MessageType;
}

const timers = new Map<number, ReturnType<typeof setTimeout>>();
let nextId = 1;

export const messageState = reactive({
  toasts: [] as ToastItem[],
});

function defaultDuration(type: MessageType) {
  return type === "error" ? 4500 : 3000;
}

export function dismissToast(id: number) {
  const timer = timers.get(id);
  if (timer) {
    clearTimeout(timer);
    timers.delete(id);
  }
  const index = messageState.toasts.findIndex((toast) => toast.id === id);
  if (index >= 0) {
    messageState.toasts.splice(index, 1);
  }
}

export function showMessage(text: string, type: MessageType = "info", durationMs?: number) {
  const id = nextId++;
  messageState.toasts.push({ id, text, type });

  const duration = durationMs ?? defaultDuration(type);
  const timer = setTimeout(() => {
    dismissToast(id);
  }, duration);
  timers.set(id, timer);
}

export function clearMessage() {
  for (const timer of timers.values()) {
    clearTimeout(timer);
  }
  timers.clear();
  messageState.toasts.splice(0, messageState.toasts.length);
}
