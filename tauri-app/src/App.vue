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
import ToastHost from "./components/ToastHost.vue";
import WatchPanel from "./components/WatchPanel.vue";
import { clearMessage } from "./composables/useMessage";
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
    <ToastHost />

    <!-- 未登录：全屏居中认证卡片 -->
    <AuthPanel v-if="!currentUser" @authenticated="onAuthenticated" />

    <!-- 已登录：左侧边栏 + 右侧内容区（桌面端经典布局） -->
    <div v-else class="app-layout">
      <aside class="sidebar">
        <div class="sidebar-brand">
          <h1>数据备份</h1>
          <p>Backup Manager</p>
        </div>

        <nav class="sidebar-nav">
          <button
            v-for="tab in tabs"
            :key="tab.id"
            type="button"
            class="sidebar-item"
            :class="{ active: activeTab === tab.id }"
            @click="activeTab = tab.id"
          >
            <!-- 侧边栏图标：用 inline SVG 保持轻量，不引入额外依赖 -->
            <span class="sidebar-icon" aria-hidden="true">
              <svg v-if="tab.id === 'backup'" xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/></svg>
              <svg v-else-if="tab.id === 'restore'" xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>
              <svg v-else-if="tab.id === 'watch'" xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>
              <svg v-else-if="tab.id === 'schedule'" xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
              <svg v-else-if="tab.id === 'manage'" xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>
              <svg v-else-if="tab.id === 'cloud'" xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round"><path d="M18 10h-1.26A8 8 0 1 0 9 20h9a5 5 0 0 0 0-10z"/></svg>
              <svg v-else xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round"><line x1="8" y1="6" x2="21" y2="6"/><line x1="8" y1="12" x2="21" y2="12"/><line x1="8" y1="18" x2="21" y2="18"/><line x1="3" y1="6" x2="3.01" y2="6"/><line x1="3" y1="12" x2="3.01" y2="12"/><line x1="3" y1="18" x2="3.01" y2="18"/></svg>
            </span>
            <span class="sidebar-label">{{ tab.label }}</span>
          </button>
        </nav>

        <div class="sidebar-footer">
          <span class="sidebar-user">{{ currentUser.username }}</span>
          <button type="button" class="secondary small" @click="logout">
            退出登录
          </button>
        </div>
      </aside>

      <div class="main-area">
        <main class="main-content">
          <BackupPanel v-show="activeTab === 'backup'" @submitted="onTaskSubmitted" />
          <RestorePanel v-show="activeTab === 'restore'" @submitted="onTaskSubmitted" />
          <WatchPanel v-show="activeTab === 'watch'" />
          <SchedulePanel v-show="activeTab === 'schedule'" />
          <BackupManagePanel v-show="activeTab === 'manage'" />
          <CloudPanel v-show="activeTab === 'cloud'" />
          <TaskList v-show="activeTab === 'tasks'" ref="taskListRef" />
        </main>
      </div>
    </div>
  </div>
</template>
