<template>
  <div class="container">
    <!-- 全局消息提示 -->
    <div v-if="message.text" class="message" :class="message.type">
      {{ message.text }}
    </div>

    <!-- 备份模块 -->
    <section class="panel">
      <h3>新建备份任务</h3>
      <div class="form-item">
        <label>源目录</label>
        <input v-model="backupForm.source" id="backupSource" type="text" />
      </div>
      <div class="form-item">
        <label>备份目标目录</label>
        <input v-model="backupForm.destination" id="backupDestination" type="text" />
      </div>
      <div class="form-item">
        <label>
          <input v-model="backupForm.compress" id="backupCompress" type="checkbox" />
          启用压缩
        </label>
      </div>
      <div class="form-item">
        <label>
          <input v-model="backupForm.encrypt" id="backupEncrypt" type="checkbox" />
          启用加密
        </label>
      </div>
      <div class="form-item">
        <label>加密密码</label>
        <input v-model="backupForm.password" id="backupPassword" type="password" />
      </div>
      <button id="backupBtn" @click="startBackup">开始备份</button>
    </section>

    <!-- 还原模块 -->
    <section class="panel">
      <h3>还原备份</h3>
      <div class="form-item">
        <label>备份文件路径</label>
        <input v-model="restoreForm.backupPath" id="restorePath" type="text" />
      </div>
      <div class="form-item">
        <label>还原目标目录</label>
        <input v-model="restoreForm.destination" id="restoreDestination" type="text" />
      </div>
      <div class="form-item">
        <label>解密密码</label>
        <input v-model="restoreForm.password" id="restorePassword" type="password" />
      </div>
      <button id="restoreBtn" @click="startRestore">开始还原</button>
    </section>

    <!-- 目录监听模块 -->
    <section class="panel">
      <h3>目录实时监听备份</h3>
      <div class="form-item">
        <label>监听源目录</label>
        <input v-model="watchForm.source" id="watchSource" type="text" />
      </div>
      <div class="form-item">
        <label>备份目标目录</label>
        <input v-model="watchForm.destination" id="watchDestination" type="text" />
      </div>
      <div class="form-item">
        <label>监听间隔（秒）</label>
        <input v-model.number="watchForm.intervalSeconds" id="watchInterval" type="number" />
      </div>
      <button id="watchStartBtn" @click="startWatch">启动监听</button>
      <button id="watchStopBtn" @click="stopWatch">停止监听</button>
    </section>

    <!-- 任务列表模块 -->
    <section class="panel">
      <div class="panel-header">
        <h3>任务列表</h3>
        <button id="refreshBtn" @click="refreshTasks">手动刷新</button>
      </div>
      <table class="task-table">
        <thead>
          <tr>
            <th>任务ID</th>
            <th>类型</th>
            <th>源路径</th>
            <th>目标路径</th>
            <th>状态</th>
            <th>信息</th>
            <th>创建时间</th>
          </tr>
        </thead>
        <tbody id="taskBody">
          <tr v-for="task in taskList" :key="task.id">
            <td>{{ task.id }}</td>
            <td>{{ escapeHtml(task.type) }}</td>
            <td>{{ escapeHtml(task.source) }}</td>
            <td>{{ escapeHtml(task.destination) }}</td>
            <td><span class="status" :class="task.status">{{ escapeHtml(task.status) }}</span></td>
            <td>{{ escapeHtml(task.message || "") }}</td>
            <td>{{ escapeHtml(task.createdAt) }}</td>
          </tr>
        </tbody>
      </table>
    </section>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onUnmounted } from 'vue';
import { request } from './utils/request';
import { escapeHtml } from './utils/escape';

// ========== 全局消息提示 ==========
const message = reactive({
  text: '',
  type: 'info'
});

function showMessage(text, type = 'info') {
  message.text = text;
  message.type = type;
}

// ========== 备份模块 ==========
const backupForm = reactive({
  source: '',
  destination: '',
  compress: false,
  encrypt: false,
  password: ''
});

async function startBackup() {
  const payload = {
    source: backupForm.source.trim(),
    destination: backupForm.destination.trim(),
    compress: backupForm.compress,
    encrypt: backupForm.encrypt,
    password: backupForm.password
  };

  if (!payload.source || !payload.destination) {
    showMessage("请输入源目录和备份目录", "error");
    return;
  }

  try {
    const result = await request("/api/backup", {
      method: "POST",
      body: JSON.stringify(payload)
    });
    showMessage(`备份任务已创建，任务编号：${result.taskId}`, "success");
    refreshTasks();
  } catch (error) {
    showMessage(error.message, "error");
  }
}

// ========== 还原模块 ==========
const restoreForm = reactive({
  backupPath: '',
  destination: '',
  password: ''
});

async function startRestore() {
  const payload = {
    backupPath: restoreForm.backupPath.trim(),
    destination: restoreForm.destination.trim(),
    password: restoreForm.password
  };

  if (!payload.backupPath || !payload.destination) {
    showMessage("请输入备份路径和还原目录", "error");
    return;
  }

  try {
    const result = await request("/api/restore", {
      method: "POST",
      body: JSON.stringify(payload)
    });
    showMessage(`还原任务已创建，任务编号：${result.taskId}`, "success");
    refreshTasks();
  } catch (error) {
    showMessage(error.message, "error");
  }
}

// ========== 目录监听模块 ==========
const watchForm = reactive({
  source: '',
  destination: '',
  intervalSeconds: 10
});

async function startWatch() {
  const payload = {
    source: watchForm.source.trim(),
    destination: watchForm.destination.trim(),
    intervalSeconds: Number(watchForm.intervalSeconds || 10)
  };

  if (!payload.source || !payload.destination) {
    showMessage("请输入监听目录和备份目录", "error");
    return;
  }

  try {
    const result = await request("/api/watch/start", {
      method: "POST",
      body: JSON.stringify(payload)
    });
    showMessage(result.message, "success");
  } catch (error) {
    showMessage(error.message, "error");
  }
}

async function stopWatch() {
  try {
    const result = await request("/api/watch/stop", {
      method: "POST",
      body: JSON.stringify({})
    });
    showMessage(result.message, "success");
  } catch (error) {
    showMessage(error.message, "error");
  }
}

// ========== 任务列表模块 ==========
const taskList = ref([]);
let refreshTimer = null;

async function refreshTasks() {
  try {
    const tasks = await request("/api/tasks");
    // 保留原代码倒序逻辑
    taskList.value = tasks.reverse();
  } catch (error) {
    showMessage(error.message, "error");
  }
}

// ========== 生命周期 ==========
onMounted(() => {
  refreshTasks();
  // 3秒自动刷新，组件销毁时自动清除，修复内存泄漏
  refreshTimer = setInterval(refreshTasks, 3000);
});

onUnmounted(() => {
  if (refreshTimer) {
    clearInterval(refreshTimer);
  }
});
</script>

<style scoped>
/* 极简基础样式，仅保证排版可用，方便后续大规模修改 */
.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
  font-family: system-ui, sans-serif;
}

.panel {
  margin-bottom: 24px;
  padding: 16px;
  border: 1px solid #eee;
  border-radius: 4px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.form-item {
  margin-bottom: 12px;
}

.form-item label {
  display: block;
  margin-bottom: 4px;
  font-size: 14px;
  color: #333;
}

.form-item input[type="text"],
.form-item input[type="password"],
.form-item input[type="number"] {
  width: 300px;
  padding: 6px 8px;
  border: 1px solid #ddd;
  border-radius: 4px;
}

button {
  padding: 6px 16px;
  margin-right: 8px;
  border: none;
  border-radius: 4px;
  background: #409eff;
  color: #fff;
  cursor: pointer;
}

button:hover {
  background: #66b1ff;
}

/* 消息提示样式 */
.message {
  padding: 10px 16px;
  margin-bottom: 16px;
  border-radius: 4px;
}

.message.success {
  background: #f0f9eb;
  color: #67c23a;
}

.message.error {
  background: #fef0f0;
  color: #f56c6c;
}

.message.info {
  background: #f4f4f5;
  color: #909399;
}

/* 任务表格 */
.task-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 14px;
}

.task-table th,
.task-table td {
  padding: 8px 12px;
  border: 1px solid #eee;
  text-align: left;
}

.status {
  padding: 2px 6px;
  border-radius: 3px;
  font-size: 12px;
}

/* 可根据后端返回的状态自行扩展颜色 */
.status.running {
  background: #ecf5ff;
  color: #409eff;
}

.status.success {
  background: #f0f9eb;
  color: #67c23a;
}

.status.failed {
  background: #fef0f0;
  color: #f56c6c;
}
</style>