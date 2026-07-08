import { reactive } from "vue";

export type MessageType = "info" | "success" | "error";

export const messageState = reactive({
  text: "",
  type: "info" as MessageType,
});

export function showMessage(text: string, type: MessageType = "info") {
  messageState.text = text;
  messageState.type = type;
}

export function clearMessage() {
  messageState.text = "";
}
