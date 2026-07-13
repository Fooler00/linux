<script setup lang="ts">
import { reactive, ref } from "vue";
import { startWatch, stopWatch } from "../api/backup";
import { showMessage } from "../composables/useMessage";
import PathPicker from "./PathPicker.vue";

const watching = ref(false);
const busy = ref(false);

const form = reactive({
  source: "",
  destination: "",
  intervalSeconds: 10,
});

async function start() {
  if (watching.value || busy.value) {
    return;
  }

  if (!form.source.trim() || !form.destination.trim()) {
    showMessage("请输入监听目录和备份目标目录", "error");
    return;
  }

  busy.value = true;

  try {
    const result = await startWatch({
      source: form.source.trim(),
      destination: form.destination.trim(),
      intervalSeconds: Number(form.intervalSeconds) || 10,
    });
    watching.value = true;
    showMessage(result.message, "success");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "启动监听失败", "error");
  } finally {
    busy.value = false;
  }
}

async function stop() {
  if (!watching.value || busy.value) {
    return;
  }

  busy.value = true;

  try {
    const result = await stopWatch();
    watching.value = false;
    showMessage(result.message, "success");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "停止监听失败", "error");
  } finally {
    busy.value = false;
  }
}
</script>

<template>
  <section class="panel">
    <h2>实时监听备份</h2>
    <!-- 状态提示条：用克制深蓝传达"监听"语义，区别于普通 hint 文本 -->
    <div class="watch-hint-bar">
      <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
        <circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/>
      </svg>
      <span>监听源目录文件变化，检测到变更后自动触发备份。</span>
    </div>
    <div class="form-grid">
      <PathPicker
        v-model="form.source"
        class="span-2"
        label="监听源目录"
        placeholder="/path/to/source"
        mode="directory"
        recent-key="source-directories"
      />
      <PathPicker
        v-model="form.destination"
        class="span-2"
        label="备份目标目录"
        placeholder="/path/to/backup"
        mode="directory"
        recent-key="backup-destinations"
      />
      <label class="field">
        <span>检测间隔（秒）</span>
        <input v-model.number="form.intervalSeconds" type="number" min="1" />
      </label>
    </div>
    <div class="actions watch-actions">
      <button
        type="button"
        class="primary"
        :disabled="watching || busy"
        @click="start"
      >
        {{ busy && !watching ? "正在启动..." : "启动监听" }}
      </button>
      <button
        type="button"
        class="secondary"
        :class="{ 'watch-stop-active': watching }"
        :disabled="!watching || busy"
        @click="stop"
      >
        {{ busy && watching ? "正在停止..." : "停止监听" }}
      </button>
    </div>
  </section>
</template>
