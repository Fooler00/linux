<script setup lang="ts">
import { onMounted, onUnmounted, ref } from "vue";
import { getTasks } from "../api/backup";
import type { Task } from "../api/types";
import { showMessage } from "../composables/useMessage";
import EmptyState from "./EmptyState.vue";
import SubmitButton from "./SubmitButton.vue";

const tasks = ref<Task[]>([]);
const loading = ref(true);
const refreshing = ref(false);
const hasLoaded = ref(false);
const lastUpdatedAt = ref("");
let timer: ReturnType<typeof setInterval> | null = null;
// 手动刷新和 3 秒轮询共用一条请求通道，避免并发请求把列表状态互相覆盖。
let pendingRequest: Promise<void> | null = null;

async function refresh(trigger: "initial" | "manual" | "poll" = "manual") {
  if (pendingRequest) {
    return pendingRequest;
  }

  if (!hasLoaded.value) {
    loading.value = true;
  }

  if (trigger === "manual") {
    refreshing.value = true;
  }

  pendingRequest = (async () => {
    try {
      const list = await getTasks();
      tasks.value = [...list].reverse();
      hasLoaded.value = true;
      lastUpdatedAt.value = new Date().toLocaleTimeString();
    } catch (error) {
      showMessage(error instanceof Error ? error.message : "获取任务列表失败", "error");
    } finally {
      loading.value = false;
      refreshing.value = false;
      pendingRequest = null;
    }
  })();

  return pendingRequest;
}

function getStatusClass(status: Task["status"]) {
  if (status === "running" || status === "success" || status === "failed") {
    return status;
  }
  return "";
}

onMounted(() => {
  refresh("initial");
  timer = setInterval(() => {
    void refresh("poll");
  }, 3000);
});

onUnmounted(() => {
  if (timer) clearInterval(timer);
});

defineExpose({ refresh });
</script>

<template>
  <section class="panel">
    <div class="panel-header">
      <div>
        <h2>任务列表</h2>
        <p v-if="lastUpdatedAt" class="hint panel-meta">最近更新：{{ lastUpdatedAt }}</p>
      </div>
      <SubmitButton variant="secondary" :loading="refreshing" :disabled="loading" @click="refresh()">
        {{ refreshing ? "正在刷新..." : "手动刷新" }}
      </SubmitButton>
    </div>

    <EmptyState
      v-if="loading && !hasLoaded"
      title="正在加载任务列表"
      description="请稍候，系统正在读取真实 /api/tasks 数据。"
    />

    <EmptyState
      v-else-if="tasks.length === 0"
      title="暂无任务"
      description="完成一次备份或还原后，任务会显示在这里。"
    />

    <div v-else class="table-wrap">
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
          <tr v-for="task in tasks" :key="task.id">
            <td>{{ task.id }}</td>
            <td>{{ task.type }}</td>
            <td class="mono">{{ task.source }}</td>
            <td class="mono">{{ task.destination }}</td>
            <td>
              <!-- 状态徽章 + 呼吸灯圆点；进行中时附加不确定进度条 -->
              <span class="status" :class="getStatusClass(task.status)">
                <span class="status-dot" aria-hidden="true"></span>
                {{ task.status }}
              </span>
              <div v-if="task.status === 'running'" class="task-progress" aria-hidden="true">
                <div class="task-progress-bar"></div>
              </div>
            </td>
            <td class="mono">{{ task.message || "-" }}</td>
            <td>{{ task.createdAt }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>
