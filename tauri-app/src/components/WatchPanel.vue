<script setup lang="ts">
import { reactive } from "vue";
import { startWatch, stopWatch } from "../api/backup";
import { showMessage } from "../composables/useMessage";
import PathPicker from "./PathPicker.vue";

const form = reactive({
  source: "",
  destination: "",
  intervalSeconds: 10,
});

async function start() {
  if (!form.source.trim() || !form.destination.trim()) {
    showMessage("请输入监听目录和备份目标目录", "error");
    return;
  }

  try {
    const result = await startWatch({
      source: form.source.trim(),
      destination: form.destination.trim(),
      intervalSeconds: Number(form.intervalSeconds) || 10,
    });
    showMessage(result.message, "success");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "启动监听失败", "error");
  }
}

async function stop() {
  try {
    const result = await stopWatch();
    showMessage(result.message, "success");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "停止监听失败", "error");
  }
}
</script>

<template>
  <section class="panel">
    <h2>实时监听备份</h2>
    <p class="hint">监听源目录文件变化，检测到变更后自动触发备份。</p>
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
    <div class="actions">
      <button type="button" class="primary" @click="start">启动监听</button>
      <button type="button" class="secondary" @click="stop">停止监听</button>
    </div>
  </section>
</template>
