<script setup lang="ts">
import { reactive, ref } from "vue";
import { startBackup } from "../api/backup";
import { showMessage } from "../composables/useMessage";
import PathInput from "./PathInput.vue";
import SubmitButton from "./SubmitButton.vue";
import { splitLines } from "../utils/format";
import { ARCHIVE_TYPE_OPTIONS, ENCRYPT_ALGO_OPTIONS } from "../utils/options";
import { validateBackupForm } from "../utils/validation";

const emit = defineEmits<{ submitted: [] }>();
// 防止同一份表单被连续点击，重复创建多个真实备份任务。
const submitting = ref(false);

const form = reactive({
  source: "",
  destination: "",
  compress: false,
  encrypt: false,
  password: "",
  archiveType: "zip",
  encryptAlgo: "aes-256-cbc",
  preserveMetadata: true,
  includeSpecialFiles: true,
  incremental: false,
  incrementalBase: "",
  includePaths: "",
  excludePaths: "",
  extensions: "",
  fileNameContains: "",
  minSize: "",
  maxSize: "",
  modifiedAfter: "",
  modifiedBefore: "",
  owner: "",
  group: "",
});

async function submit() {
  if (submitting.value) {
    return;
  }

  const validation = validateBackupForm(form);
  if (!validation.valid) {
    showMessage(validation.message ?? "请检查备份表单", "error");
    return;
  }

  submitting.value = true;

  // 高级筛选仍按后端既有字段组织，只把非空值拼回 payload，避免改动协议。
  const filter: Record<string, unknown> = {
    preserveMetadata: form.preserveMetadata,
    includeSpecialFiles: form.includeSpecialFiles,
  };

  const includePaths = splitLines(form.includePaths);
  const excludePaths = splitLines(form.excludePaths);
  const extensions = splitLines(form.extensions);

  if (includePaths.length) filter.includePaths = includePaths;
  if (excludePaths.length) filter.excludePaths = excludePaths;
  if (extensions.length) filter.extensions = extensions;
  if (form.fileNameContains.trim()) filter.fileNameContains = form.fileNameContains.trim();
  if (form.minSize) filter.minSize = Number(form.minSize);
  if (form.maxSize) filter.maxSize = Number(form.maxSize);
  if (form.modifiedAfter) filter.modifiedAfter = form.modifiedAfter;
  if (form.modifiedBefore) filter.modifiedBefore = form.modifiedBefore;
  if (form.owner.trim()) filter.owner = form.owner.trim();
  if (form.group.trim()) filter.group = form.group.trim();

  try {
    const result = await startBackup({
      source: form.source.trim(),
      destination: form.destination.trim(),
      compress: form.compress,
      encrypt: form.encrypt,
      password: form.password,
      archiveType: form.archiveType,
      encryptAlgo: form.encryptAlgo,
      preserveMetadata: form.preserveMetadata,
      includeSpecialFiles: form.includeSpecialFiles,
      incremental: form.incremental,
      incrementalBase: form.incrementalBase.trim() || undefined,
      filter,
    });
    showMessage(`备份任务已创建，任务编号：${result.taskId}`, "success");
    emit("submitted");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "备份失败", "error");
  } finally {
    submitting.value = false;
  }
}
</script>

<template>
  <section class="panel">
    <h2>手动备份</h2>
    <div class="form-grid">
      <PathInput
        v-model="form.source"
        class="span-2"
        label="源目录"
        placeholder="/path/to/source"
        :disabled="submitting"
      />
      <PathInput
        v-model="form.destination"
        class="span-2"
        label="备份目标目录"
        placeholder="/path/to/backup"
        :disabled="submitting"
      />

      <label class="field">
        <span>归档类型</span>
        <select v-model="form.archiveType">
          <option
            v-for="option in ARCHIVE_TYPE_OPTIONS"
            :key="option.value"
            :value="option.value"
          >
            {{ option.label }}
          </option>
        </select>
      </label>
      <label class="field">
        <span>加密算法</span>
        <select v-model="form.encryptAlgo">
          <option
            v-for="option in ENCRYPT_ALGO_OPTIONS"
            :key="option.value"
            :value="option.value"
          >
            {{ option.label }}
          </option>
        </select>
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
        <label class="checkbox">
          <input v-model="form.preserveMetadata" type="checkbox" />
          <span>保留元数据</span>
        </label>
        <label class="checkbox">
          <input v-model="form.includeSpecialFiles" type="checkbox" />
          <span>包含特殊文件</span>
        </label>
        <label class="checkbox">
          <input v-model="form.incremental" type="checkbox" />
          <span>增量备份</span>
        </label>
      </div>

      <label v-if="form.encrypt" class="field span-2">
        <span>加密密码</span>
        <input v-model="form.password" type="password" />
      </label>
      <label v-if="form.incremental" class="field span-2">
        <span>增量基线目录</span>
        <input v-model="form.incrementalBase" type="text" placeholder="上一次完整备份目录" />
      </label>

      <details class="span-2 filter-details">
        <summary>高级筛选</summary>
        <div class="form-grid nested">
          <label class="field span-2">
            <span>包含路径（逗号或换行分隔）</span>
            <textarea v-model="form.includePaths" rows="2" />
          </label>
          <label class="field span-2">
            <span>排除路径</span>
            <textarea v-model="form.excludePaths" rows="2" />
          </label>
          <label class="field">
            <span>扩展名</span>
            <input v-model="form.extensions" placeholder="txt, pdf" />
          </label>
          <label class="field">
            <span>文件名包含</span>
            <input v-model="form.fileNameContains" />
          </label>
          <label class="field">
            <span>最小大小（字节）</span>
            <input v-model="form.minSize" type="number" min="0" />
          </label>
          <label class="field">
            <span>最大大小（字节）</span>
            <input v-model="form.maxSize" type="number" min="0" />
          </label>
          <label class="field">
            <span>修改时间晚于</span>
            <input v-model="form.modifiedAfter" placeholder="2026-01-01 00:00:00" />
          </label>
          <label class="field">
            <span>修改时间早于</span>
            <input v-model="form.modifiedBefore" placeholder="2026-12-31 23:59:59" />
          </label>
          <label class="field">
            <span>属主名</span>
            <input v-model="form.owner" />
          </label>
          <label class="field">
            <span>属组名</span>
            <input v-model="form.group" />
          </label>
        </div>
      </details>
    </div>

    <div class="actions">
      <SubmitButton :loading="submitting" @click="submit">
        {{ submitting ? "正在提交备份任务..." : "开始备份" }}
      </SubmitButton>
    </div>
  </section>
</template>
