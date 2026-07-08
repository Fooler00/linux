<script setup lang="ts">
import { reactive, ref } from "vue";
import { getMetadata, listBackups, pruneBackups } from "../api/backup";
import type { BackupItem, BackupMetadata } from "../api/types";
import { showMessage } from "../composables/useMessage";
import EmptyState from "./EmptyState.vue";
import PathInput from "./PathInput.vue";
import SubmitButton from "./SubmitButton.vue";
import { formatBytes, formatTimestamp } from "../utils/format";
import { validateBackupManageDestination } from "../utils/validation";

const destination = ref("");
const backups = ref<BackupItem[]>([]);
const metadata = ref<BackupMetadata | null>(null);
const selectedPath = ref("");
const querying = ref(false);
const pruning = ref(false);
// metadata 读取是按单条备份目录触发的，用路径值防止重复点击叠加请求。
const loadingMetadataPath = ref("");
const hasQueried = ref(false);

const pruneForm = reactive({
  maxBackups: 5,
  maxAgeDays: 0,
});

async function queryBackups() {
  if (querying.value) {
    return;
  }

  const validation = validateBackupManageDestination(destination.value);
  if (!validation.valid) {
    showMessage(validation.message ?? "请检查备份目录", "error");
    return;
  }

  querying.value = true;

  try {
    backups.value = await listBackups(destination.value.trim());
    hasQueried.value = true;
    // 查询成功后清掉旧 metadata，避免列表已经切换但下方仍显示上一次目录的信息。
    metadata.value = null;
    selectedPath.value = "";
    showMessage(`找到 ${backups.value.length} 个备份`, "success");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "查询备份列表失败", "error");
  } finally {
    querying.value = false;
  }
}

async function viewMetadata(path: string) {
  if (loadingMetadataPath.value) {
    return;
  }

  // metadata.json 来自真实备份目录，切换条目时先清空旧展示，避免用户误读为同一路径内容。
  selectedPath.value = path;
  metadata.value = null;
  loadingMetadataPath.value = path;

  try {
    metadata.value = await getMetadata(path);
  } catch (error) {
    metadata.value = null;
    showMessage(error instanceof Error ? error.message : "读取元数据失败", "error");
  } finally {
    loadingMetadataPath.value = "";
  }
}

async function prune() {
  if (pruning.value) {
    return;
  }

  const validation = validateBackupManageDestination(destination.value);
  if (!validation.valid) {
    showMessage(validation.message ?? "请检查备份目录", "error");
    return;
  }

  pruning.value = true;

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
  } finally {
    pruning.value = false;
  }
}

function getMetadataText(value: string | number | null | undefined) {
  return value === "" || value === null || value === undefined ? "-" : String(value);
}

function getMetadataBytes(value: number | null | undefined) {
  return value === null || value === undefined ? "-" : formatBytes(value);
}

function getMetadataBoolean(value: boolean | null | undefined) {
  // metadata.json 可能来自历史备份；字段缺失时统一兜底为 "-"，避免页面因为旧数据崩掉。
  return value === null || value === undefined ? "-" : value ? "是" : "否";
}
</script>

<template>
  <section class="panel">
    <h2>备份管理与淘汰</h2>
    <div class="form-grid">
      <PathInput
        v-model="destination"
        class="span-2"
        label="备份目标目录"
        placeholder="/path/to/backup"
        :disabled="querying || pruning"
      />
      <label class="field">
        <span>最多保留数量（0=不限）</span>
        <input v-model.number="pruneForm.maxBackups" type="number" min="0" :disabled="pruning" />
      </label>
      <label class="field">
        <span>最多保留天数（0=不限）</span>
        <input v-model.number="pruneForm.maxAgeDays" type="number" min="0" :disabled="pruning" />
      </label>
    </div>
    <div class="actions">
      <SubmitButton :loading="querying" :disabled="pruning" @click="queryBackups">
        {{ querying ? "正在查询..." : "查询备份" }}
      </SubmitButton>
      <SubmitButton variant="danger" :loading="pruning" :disabled="querying" @click="prune">
        {{ pruning ? "正在清理..." : "执行淘汰" }}
      </SubmitButton>
    </div>

    <p v-if="!hasQueried" class="hint">输入备份目录后可查询备份列表，并查看对应 metadata。</p>

    <EmptyState
      v-else-if="backups.length === 0"
      title="暂无备份记录"
      description="请确认目录是否正确，或先执行一次备份。"
    />

    <div v-else class="table-wrap">
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
          <tr v-for="item in backups" :key="item.path">
            <td class="mono">{{ item.name }}</td>
            <td>{{ formatBytes(item.size) }}</td>
            <td>{{ formatTimestamp(item.timestamp) }}</td>
            <td>
              <SubmitButton
                variant="secondary"
                class="small"
                :loading="loadingMetadataPath === item.path"
                :disabled="Boolean(loadingMetadataPath) && loadingMetadataPath !== item.path"
                @click="viewMetadata(item.path)"
              >
                {{ loadingMetadataPath === item.path ? "读取中..." : "查看元数据" }}
              </SubmitButton>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div v-if="metadata" class="metadata-box">
      <h3>元数据：{{ selectedPath }}</h3>
      <dl class="metadata-list">
        <div><dt>源目录</dt><dd>{{ getMetadataText(metadata.source) }}</dd></div>
        <div><dt>备份时间</dt><dd>{{ getMetadataText(metadata.backupTime) }}</dd></div>
        <div><dt>归档类型</dt><dd>{{ getMetadataText(metadata.archiveType) }}</dd></div>
        <div><dt>加密算法</dt><dd>{{ getMetadataText(metadata.encryptAlgo) }}</dd></div>
        <div><dt>文件数</dt><dd>{{ getMetadataText(metadata.copiedFiles) }}</dd></div>
        <div><dt>大小</dt><dd>{{ getMetadataBytes(metadata.size) }}</dd></div>
        <div><dt>增量备份</dt><dd>{{ getMetadataBoolean(metadata.incremental) }}</dd></div>
        <div><dt>保留元数据</dt><dd>{{ getMetadataBoolean(metadata.preserveMetadata) }}</dd></div>
      </dl>
    </div>
  </section>
</template>
