<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { open } from "@tauri-apps/plugin-dialog";

const RECENT_LIMIT = 5;
const STORAGE_PREFIX = "path-picker-recent:";
const RECENT_EVENT = "path-picker-recent-updated";

const model = defineModel<string>({ required: true });
const sourcesModel = defineModel<string[]>("sources", { default: () => [] });

const props = withDefaults(
  defineProps<{
    label: string;
    placeholder?: string;
    disabled?: boolean;
    mode: "directory" | "file" | "file-or-directory";
    recentKey: string;
    allowMultipleFiles?: boolean;
  }>(),
  {
    placeholder: "",
    disabled: false,
    allowMultipleFiles: false,
  }
);

const recentSelection = ref("");
const recentPaths = ref<string[]>([]);

const multiFileSummary = computed(() => {
  const count = sourcesModel.value.length;
  if (count === 0) {
    return "";
  }
  return `已选 ${count} 个文件`;
});

onMounted(() => {
  syncRecentPaths();
  window.addEventListener(RECENT_EVENT, handleRecentPathsUpdated as EventListener);
});

onBeforeUnmount(() => {
  window.removeEventListener(RECENT_EVENT, handleRecentPathsUpdated as EventListener);
});

function clearSources() {
  sourcesModel.value = [];
}

function removeSource(path: string) {
  sourcesModel.value = sourcesModel.value.filter((item) => item !== path);
}

async function pickPath(pickMode: "directory" | "file") {
  if (props.disabled) {
    return;
  }

  const selected = await open({
    directory: pickMode === "directory",
    multiple: false,
  });

  const nextPath = Array.isArray(selected) ? selected[0] : selected;

  if (typeof nextPath !== "string" || !nextPath.trim()) {
    return;
  }

  model.value = nextPath;
  clearSources();
  saveRecentPath(props.recentKey, nextPath);
  recentSelection.value = "";
}

async function pickMultipleFiles() {
  if (props.disabled) {
    return;
  }

  const selected = await open({
    directory: false,
    multiple: true,
  });

  if (!selected) {
    return;
  }

  const paths = (Array.isArray(selected) ? selected : [selected])
    .filter((item): item is string => typeof item === "string" && item.trim().length > 0)
    .map((item) => item.trim());

  if (paths.length === 0) {
    return;
  }

  const merged = [...new Set([...sourcesModel.value, ...paths])];
  sourcesModel.value = merged;
  model.value = "";
  recentSelection.value = "";

  for (const path of paths) {
    saveRecentPath(props.recentKey, path);
  }
}

function applyRecentPath() {
  if (!recentSelection.value) {
    return;
  }

  model.value = recentSelection.value;
  clearSources();
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
  <label class="field path-picker" :class="{ 'path-picker-multi-active': sourcesModel.length > 0 }">
    <span>{{ label }}</span>

    <!-- 多文件模式：展示摘要与 Chip 列表，避免单行输入框挤满长路径 -->
    <div v-if="sourcesModel.length > 0" class="path-picker-multi">
      <p class="path-picker-multi-summary">{{ multiFileSummary }}</p>
      <ul class="path-picker-chip-list">
        <li v-for="path in sourcesModel" :key="path" class="path-picker-chip">
          <span class="path-picker-chip-text mono" :title="path">{{ path }}</span>
          <button
            type="button"
            class="path-picker-chip-remove"
            :disabled="disabled"
            aria-label="移除文件"
            @click="removeSource(path)"
          >
            ×
          </button>
        </li>
      </ul>
    </div>

    <div v-else class="path-picker-row">
      <input
        v-model="model"
        type="text"
        :placeholder="placeholder"
        :disabled="disabled"
      />
      <template v-if="mode === 'file-or-directory'">
        <button
          type="button"
          class="secondary path-picker-button"
          :disabled="disabled"
          @click="pickPath('file')"
        >
          选择文件
        </button>
        <button
          v-if="allowMultipleFiles"
          type="button"
          class="secondary path-picker-button"
          :disabled="disabled"
          @click="pickMultipleFiles"
        >
          选择多个文件
        </button>
        <button
          type="button"
          class="secondary path-picker-button"
          :disabled="disabled"
          @click="pickPath('directory')"
        >
          选择目录
        </button>
      </template>
      <button
        v-else
        type="button"
        class="secondary path-picker-button"
        :disabled="disabled"
        @click="pickPath(mode)"
      >
        选择
      </button>
    </div>

    <div v-if="sourcesModel.length > 0" class="path-picker-row path-picker-multi-actions">
      <button
        v-if="allowMultipleFiles"
        type="button"
        class="secondary path-picker-button"
        :disabled="disabled"
        @click="pickMultipleFiles"
      >
        继续添加文件
      </button>
      <button
        type="button"
        class="secondary path-picker-button"
        :disabled="disabled"
        @click="clearSources"
      >
        清空文件列表
      </button>
    </div>

    <div v-if="recentPaths.length > 0 && sourcesModel.length === 0" class="path-picker-recent">
      <select v-model="recentSelection" :disabled="disabled" @change="applyRecentPath">
        <option value="">最近路径</option>
        <option v-for="path in recentPaths" :key="path" :value="path">
          {{ path }}
        </option>
      </select>
    </div>
  </label>
</template>
