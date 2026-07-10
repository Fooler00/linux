<script setup lang="ts">
import { ref } from "vue";
import type { AuthUser } from "./api/types";
import CloudPanel from "./components/CloudPanel.vue";
import BackupManagePanel from "./components/BackupManagePanel.vue";
import AuthPanel from "./components/AuthPanel.vue";
import BackupPanel from "./components/BackupPanel.vue";
import RestorePanel from "./components/RestorePanel.vue";
import SchedulePanel from "./components/SchedulePanel.vue";
import TaskList from "./components/TaskList.vue";
import WatchPanel from "./components/WatchPanel.vue";
import { clearMessage, messageState } from "./composables/useMessage";
import { clearSession, readSession, saveSession } from "./utils/session";

const tabs = [
  { id: "backup", label: "手动备份" },
  { id: "restore", label: "备份还原" },
  { id: "watch", label: "实时监听" },
  { id: "schedule", label: "定时备份" },
  { id: "manage", label: "备份管理" },
  { id: "cloud", label: "云存储" },
  { id: "tasks", label: "任务列表" },
] as const;

type TabId = (typeof tabs)[number]["id"];

const activeTab = ref<TabId>("backup");
const taskListRef = ref<InstanceType<typeof TaskList> | null>(null);
const currentUser = ref<AuthUser | null>(readSession());

function onAuthenticated(user: AuthUser) {
  saveSession(user);
  currentUser.value = user;
}

function logout() {
  clearSession();
  currentUser.value = null;
  activeTab.value = "backup";
  clearMessage();
}

function onTaskSubmitted() {
  taskListRef.value?.refresh();
}
</script>

<template>
  <div class="app-shell">
    <header class="app-header">
      <div>
        <h1>数据备份</h1>
        <p>基于 Tauri + Vue 的备份管理界面</p>
      </div>
      <div v-if="currentUser" class="header-session">
        <span class="header-user">当前用户：{{ currentUser.username }}</span>
        <button type="button" class="secondary small" @click="logout">
          退出登录
        </button>
      </div>
    </header>

    <div
      v-if="messageState.text"
      class="message-bar"
      :class="messageState.type"
      @click="clearMessage"
    >
      {{ messageState.text }}
    </div>

    <AuthPanel v-if="!currentUser" @authenticated="onAuthenticated" />

    <template v-else>
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
        <CloudPanel v-show="activeTab === 'cloud'" />
        <TaskList v-show="activeTab === 'tasks'" ref="taskListRef" />
      </main>
    </template>
  </div>
</template>
