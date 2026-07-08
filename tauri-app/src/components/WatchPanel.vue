<script setup lang="ts">
import { reactive } from "vue";
import { startWatch, stopWatch } from "../api/backup";
import { showMessage } from "../composables/useMessage";

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
      <label class="field span-2">
        <span>监听源目录</span>
        <input v-model="form.source" type="text" />
      </label>
      <label class="field span-2">
        <span>备份目标目录</span>
        <input v-model="form.destination" type="text" />
      </label>
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
