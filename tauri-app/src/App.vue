<script setup lang="ts">
import { ref } from "vue";
import BackupManagePanel from "./components/BackupManagePanel.vue";
import BackupPanel from "./components/BackupPanel.vue";
import RestorePanel from "./components/RestorePanel.vue";
import SchedulePanel from "./components/SchedulePanel.vue";
import TaskList from "./components/TaskList.vue";
import WatchPanel from "./components/WatchPanel.vue";
import { clearMessage, messageState } from "./composables/useMessage";

const tabs = [
  { id: "backup", label: "手动备份" },
  { id: "restore", label: "备份还原" },
  { id: "watch", label: "实时监听" },
  { id: "schedule", label: "定时备份" },
  { id: "manage", label: "备份管理" },
  { id: "tasks", label: "任务列表" },
] as const;

type TabId = (typeof tabs)[number]["id"];

const activeTab = ref<TabId>("backup");
const taskListRef = ref<InstanceType<typeof TaskList> | null>(null);

function onTaskSubmitted() {
  taskListRef.value?.refresh();
}
</script>

<template>
  <div class="app-shell">
    <header class="app-header">
      <h1>数据备份</h1>
      <p>基于 Tauri + Vue 的备份管理界面</p>
    </header>

    <div
      v-if="messageState.text"
      class="message-bar"
      :class="messageState.type"
      @click="clearMessage"
    >
      {{ messageState.text }}
    </div>

    <nav class="tabs">
      <button
        v-for="tab in tabs"
        :key="tab.id"
        type="button"
        class="tab"
        :class="{ active: activeTab === tab.id }"
        @click="activeTab = tab.id"
      >
        {{ tab.label }}
      </button>
    </nav>

    <main>
      <BackupPanel v-show="activeTab === 'backup'" @submitted="onTaskSubmitted" />
      <RestorePanel v-show="activeTab === 'restore'" @submitted="onTaskSubmitted" />
      <WatchPanel v-show="activeTab === 'watch'" />
      <SchedulePanel v-show="activeTab === 'schedule'" />
      <BackupManagePanel v-show="activeTab === 'manage'" />
      <TaskList v-show="activeTab === 'tasks'" ref="taskListRef" />
    </main>
  </div>
</template>
