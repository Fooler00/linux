<script setup lang="ts">
import { reactive, ref } from "vue";
import { getMetadata, listBackups, pruneBackups } from "../api/backup";
import type { BackupItem, BackupMetadata } from "../api/types";
import { showMessage } from "../composables/useMessage";
import { formatBytes, formatTimestamp } from "../utils/format";

const destination = ref("");
const backups = ref<BackupItem[]>([]);
const metadata = ref<BackupMetadata | null>(null);
const selectedPath = ref("");

const pruneForm = reactive({
  maxBackups: 5,
  maxAgeDays: 0,
});

async function queryBackups() {
  if (!destination.value.trim()) {
    showMessage("请输入备份目标目录", "error");
    return;
  }

  try {
    backups.value = await listBackups(destination.value.trim());
    metadata.value = null;
    selectedPath.value = "";
    showMessage(`找到 ${backups.value.length} 个备份`, "success");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "查询备份列表失败", "error");
  }
}

async function viewMetadata(path: string) {
  selectedPath.value = path;
  try {
    metadata.value = await getMetadata(path);
  } catch (error) {
    metadata.value = null;
    showMessage(error instanceof Error ? error.message : "读取元数据失败", "error");
  }
}

async function prune() {
  if (!destination.value.trim()) {
    showMessage("请输入备份目标目录", "error");
    return;
  }

  try {
    const result = await pruneBackups(
      destination.value.trim(),
      Number(pruneForm.maxBackups) || 0,
      Number(pruneForm.maxAgeDays) || 0
    );
    showMessage(result.message, "success");
    await queryBackups();
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "淘汰备份失败", "error");
  }
}
</script>

<template>
  <section class="panel">
    <h2>备份管理与淘汰</h2>
    <div class="form-grid">
      <label class="field span-2">
        <span>备份目标目录</span>
        <input v-model="destination" type="text" placeholder="/path/to/backup" />
      </label>
      <label class="field">
        <span>最多保留数量（0=不限）</span>
        <input v-model.number="pruneForm.maxBackups" type="number" min="0" />
      </label>
      <label class="field">
        <span>最多保留天数（0=不限）</span>
        <input v-model.number="pruneForm.maxAgeDays" type="number" min="0" />
      </label>
    </div>
    <div class="actions">
      <button type="button" class="primary" @click="queryBackups">查询备份</button>
      <button type="button" class="danger" @click="prune">执行淘汰</button>
    </div>

    <div class="table-wrap">
      <table class="data-table">
        <thead>
          <tr>
            <th>名称</th>
            <th>大小</th>
            <th>时间</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-if="backups.length === 0">
            <td colspan="4" class="empty">暂无备份记录</td>
          </tr>
          <tr v-for="item in backups" :key="item.path">
            <td class="mono">{{ item.name }}</td>
            <td>{{ formatBytes(item.size) }}</td>
            <td>{{ formatTimestamp(item.timestamp) }}</td>
            <td>
              <button type="button" class="secondary small" @click="viewMetadata(item.path)">
                查看元数据
              </button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div v-if="metadata" class="metadata-box">
      <h3>元数据：{{ selectedPath }}</h3>
      <dl class="metadata-list">
        <div><dt>源目录</dt><dd>{{ metadata.source }}</dd></div>
        <div><dt>备份时间</dt><dd>{{ metadata.backupTime }}</dd></div>
        <div><dt>归档类型</dt><dd>{{ metadata.archiveType }}</dd></div>
        <div><dt>加密算法</dt><dd>{{ metadata.encryptAlgo }}</dd></div>
        <div><dt>文件数</dt><dd>{{ metadata.copiedFiles }}</dd></div>
        <div><dt>大小</dt><dd>{{ formatBytes(metadata.size) }}</dd></div>
        <div><dt>增量备份</dt><dd>{{ metadata.incremental ? "是" : "否" }}</dd></div>
        <div><dt>保留元数据</dt><dd>{{ metadata.preserveMetadata ? "是" : "否" }}</dd></div>
      </dl>
    </div>
  </section>
</template>
