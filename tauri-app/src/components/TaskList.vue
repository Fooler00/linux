<script setup lang="ts">
import { onMounted, onUnmounted, ref } from "vue";
import { getTasks } from "../api/backup";
import type { Task } from "../api/types";
import { showMessage } from "../composables/useMessage";

const tasks = ref<Task[]>([]);
let timer: ReturnType<typeof setInterval> | null = null;

async function refresh() {
  try {
    const list = await getTasks();
    tasks.value = [...list].reverse();
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "获取任务列表失败", "error");
  }
}

onMounted(() => {
  refresh();
  timer = setInterval(refresh, 3000);
});

onUnmounted(() => {
  if (timer) clearInterval(timer);
});

defineExpose({ refresh });
</script>

<template>
  <section class="panel">
    <div class="panel-header">
      <h2>任务列表</h2>
      <button type="button" class="secondary" @click="refresh">手动刷新</button>
    </div>
    <div class="table-wrap">
      <table class="data-table">
        <thead>
          <tr>
            <th>ID</th>
            <th>类型</th>
            <th>源路径</th>
            <th>目标路径</th>
            <th>状态</th>
            <th>信息</th>
            <th>创建时间</th>
          </tr>
        </thead>
        <tbody>
          <tr v-if="tasks.length === 0">
            <td colspan="7" class="empty">暂无任务</td>
          </tr>
          <tr v-for="task in tasks" :key="task.id">
            <td>{{ task.id }}</td>
            <td>{{ task.type }}</td>
            <td class="mono">{{ task.source }}</td>
            <td class="mono">{{ task.destination }}</td>
            <td>
              <span class="status" :class="task.status">{{ task.status }}</span>
            </td>
            <td class="mono">{{ task.message || "-" }}</td>
            <td>{{ task.createdAt }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>
