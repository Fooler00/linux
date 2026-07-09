<script setup lang="ts">
import { onBeforeUnmount, onMounted, ref } from "vue";
import { open } from "@tauri-apps/plugin-dialog";

const RECENT_LIMIT = 5;
const STORAGE_PREFIX = "path-picker-recent:";
const RECENT_EVENT = "path-picker-recent-updated";

const model = defineModel<string>({ required: true });

const props = withDefaults(
  defineProps<{
    label: string;
    placeholder?: string;
    disabled?: boolean;
    mode: "directory" | "file";
    recentKey: string;
  }>(),
  {
    placeholder: "",
    disabled: false,
  }
);

const recentSelection = ref("");
const recentPaths = ref<string[]>([]);

onMounted(() => {
  syncRecentPaths();
  window.addEventListener(RECENT_EVENT, handleRecentPathsUpdated as EventListener);
});

onBeforeUnmount(() => {
  window.removeEventListener(RECENT_EVENT, handleRecentPathsUpdated as EventListener);
});

async function pickPath() {
  if (props.disabled) {
    return;
  }

  const selected = await open({
    directory: props.mode === "directory",
    multiple: false,
  });

  const nextPath = Array.isArray(selected) ? selected[0] : selected;

  if (typeof nextPath !== "string" || !nextPath.trim()) {
    return;
  }

  model.value = nextPath;
  saveRecentPath(props.recentKey, nextPath);
  recentSelection.value = "";
}

function applyRecentPath() {
  if (!recentSelection.value) {
    return;
  }

  model.value = recentSelection.value;
  recentSelection.value = "";
}

function readRecentPaths(recentKey: string): string[] {
  const raw = localStorage.getItem(`${STORAGE_PREFIX}${recentKey}`);

  if (!raw) {
    return [];
  }

  try {
    const parsed = JSON.parse(raw) as unknown;

    if (!Array.isArray(parsed)) {
      return [];
    }

    return parsed.filter((item): item is string => typeof item === "string" && item.trim().length > 0);
  } catch {
    return [];
  }
}

function saveRecentPath(recentKey: string, path: string) {
  const nextPath = path.trim();

  if (!nextPath) {
    return;
  }

  const deduped = [nextPath, ...readRecentPaths(recentKey).filter((item) => item !== nextPath)].slice(
    0,
    RECENT_LIMIT
  );

  localStorage.setItem(`${STORAGE_PREFIX}${recentKey}`, JSON.stringify(deduped));
  window.dispatchEvent(new CustomEvent(RECENT_EVENT, { detail: { recentKey } }));
}

function syncRecentPaths() {
  recentPaths.value = readRecentPaths(props.recentKey);
}

function handleRecentPathsUpdated(event: Event) {
  const recentEvent = event as CustomEvent<{ recentKey?: string }>;

  if (recentEvent.detail?.recentKey !== props.recentKey) {
    return;
  }

  syncRecentPaths();
}
</script>

<template>
  <label class="field path-picker">
    <span>{{ label }}</span>
    <div class="path-picker-row">
      <input
        v-model="model"
        type="text"
        :placeholder="placeholder"
        :disabled="disabled"
      />
      <button
        type="button"
        class="secondary path-picker-button"
        :disabled="disabled"
        @click="pickPath"
      >
        选择
      </button>
    </div>

    <div v-if="recentPaths.length > 0" class="path-picker-recent">
      <select v-model="recentSelection" :disabled="disabled" @change="applyRecentPath">
        <option value="">最近路径</option>
        <option v-for="path in recentPaths" :key="path" :value="path">
          {{ path }}
        </option>
      </select>
    </div>
  </label>
</template>
