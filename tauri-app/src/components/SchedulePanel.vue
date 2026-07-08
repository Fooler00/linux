<script setup lang="ts">
import { onMounted, reactive, ref } from "vue";
import { getSchedules, startSchedule, stopSchedule } from "../api/backup";
import type { Schedule } from "../api/types";
import { showMessage } from "../composables/useMessage";

const schedules = ref<Schedule[]>([]);

const form = reactive({
  source: "",
  destination: "",
  intervalSeconds: 3600,
  maxBackups: 0,
  maxAgeDays: 0,
  compress: false,
  encrypt: false,
  password: "",
});

async function refresh() {
  try {
    schedules.value = await getSchedules();
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "获取调度列表失败", "error");
  }
}

async function submit() {
  if (!form.source.trim() || !form.destination.trim()) {
    showMessage("请输入源目录和备份目标目录", "error");
    return;
  }

  try {
    const result = await startSchedule({
      source: form.source.trim(),
      destination: form.destination.trim(),
      intervalSeconds: Number(form.intervalSeconds) || 3600,
      maxBackups: Number(form.maxBackups) || 0,
      maxAgeDays: Number(form.maxAgeDays) || 0,
      compress: form.compress,
      encrypt: form.encrypt,
      password: form.password,
    });
    showMessage(`${result.message}（ID: ${result.scheduleId}）`, "success");
    await refresh();
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "启动定时备份失败", "error");
  }
}

async function stop(id: number) {
  try {
    const result = await stopSchedule(id);
    showMessage(result.message, result.success ? "success" : "error");
    await refresh();
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "停止调度失败", "error");
  }
}

function formatInterval(seconds: number) {
  if (seconds >= 3600 && seconds % 3600 === 0) {
    return `${seconds / 3600} 小时`;
  }
  if (seconds >= 60 && seconds % 60 === 0) {
    return `${seconds / 60} 分钟`;
  }
  return `${seconds} 秒`;
}

onMounted(refresh);
</script>

<template>
  <section class="panel">
    <h2>定时备份</h2>
    <div class="form-grid">
      <label class="field span-2">
        <span>源目录</span>
        <input v-model="form.source" type="text" />
      </label>
      <label class="field span-2">
        <span>备份目标目录</span>
        <input v-model="form.destination" type="text" />
      </label>
      <label class="field">
        <span>定时间隔（秒）</span>
        <input v-model.number="form.intervalSeconds" type="number" min="1" />
      </label>
      <label class="field">
        <span>最多保留备份数（0=不限）</span>
        <input v-model.number="form.maxBackups" type="number" min="0" />
      </label>
      <label class="field">
        <span>最多保留天数（0=不限）</span>
        <input v-model.number="form.maxAgeDays" type="number" min="0" />
      </label>
      <div class="checkbox-group">
        <label class="checkbox">
          <input v-model="form.compress" type="checkbox" />
          <span>启用压缩</span>
        </label>
        <label class="checkbox">
          <input v-model="form.encrypt" type="checkbox" />
          <span>启用加密</span>
        </label>
      </div>
      <label v-if="form.encrypt" class="field span-2">
        <span>加密密码</span>
        <input v-model="form.password" type="password" />
      </label>
    </div>
    <div class="actions">
      <button type="button" class="primary" @click="submit">启动定时备份</button>
      <button type="button" class="secondary" @click="refresh">刷新列表</button>
    </div>

    <div class="table-wrap">
      <table class="data-table">
        <thead>
          <tr>
            <th>ID</th>
            <th>源目录</th>
            <th>目标目录</th>
            <th>间隔</th>
            <th>保留策略</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-if="schedules.length === 0">
            <td colspan="6" class="empty">暂无运行中的定时任务</td>
          </tr>
          <tr v-for="item in schedules" :key="item.id">
            <td>{{ item.id }}</td>
            <td class="mono">{{ item.source }}</td>
            <td class="mono">{{ item.destination }}</td>
            <td>{{ formatInterval(item.intervalSeconds) }}</td>
            <td>
              <span v-if="item.maxBackups">最多 {{ item.maxBackups }} 个</span>
              <span v-if="item.maxBackups && item.maxAgeDays"> / </span>
              <span v-if="item.maxAgeDays">最多 {{ item.maxAgeDays }} 天</span>
              <span v-if="!item.maxBackups && !item.maxAgeDays">不限</span>
            </td>
            <td>
              <button type="button" class="danger small" @click="stop(item.id)">停止</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>
