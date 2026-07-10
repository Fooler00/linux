<script setup lang="ts">
import { onMounted, reactive, ref } from "vue";
import { save } from "@tauri-apps/plugin-dialog";
import {
  deleteCloudFile,
  downloadCloudFile,
  listCloudFiles,
  uploadCloudFile,
} from "../api/cloud";
import type { CloudFile } from "../api/types";
import { showMessage } from "../composables/useMessage";
import { readCloudToken, saveCloudToken } from "../utils/cloudToken";
import { formatBytes, formatTimestamp } from "../utils/format";
import EmptyState from "./EmptyState.vue";
import PathPicker from "./PathPicker.vue";
import SubmitButton from "./SubmitButton.vue";

const remoteDir = ref("");
const cloudToken = ref(readCloudToken());
const files = ref<CloudFile[]>([]);
const hasQueried = ref(false);
const listing = ref(false);
const uploading = ref(false);
const deletingPath = ref("");
const downloadingPath = ref("");

const uploadForm = reactive({
  localPath: "",
  remotePath: "",
});

function joinRemotePath(dir: string, name: string) {
  const normalizedDir = dir.trim().replace(/^\/+|\/+$/g, "");
  const normalizedName = name.trim().replace(/^\/+/, "");

  if (!normalizedDir) {
    return normalizedName;
  }

  return `${normalizedDir}/${normalizedName}`;
}

function saveToken() {
  saveCloudToken(cloudToken.value);
  showMessage("云存储 Token 已保存", "success");
}

async function refreshList(showSuccess = false) {
  if (listing.value) {
    return;
  }

  listing.value = true;

  try {
    files.value = await listCloudFiles(remoteDir.value.trim());
    hasQueried.value = true;
    if (showSuccess) {
      showMessage(`找到 ${files.value.length} 个云端文件`, "success");
    }
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "获取云端列表失败", "error");
  } finally {
    listing.value = false;
  }
}

async function uploadFile() {
  if (uploading.value) {
    return;
  }

  if (!uploadForm.localPath.trim()) {
    showMessage("请选择要上传的本地文件", "error");
    return;
  }

  if (!uploadForm.remotePath.trim()) {
    showMessage("请输入云端保存路径", "error");
    return;
  }

  uploading.value = true;

  try {
    const result = await uploadCloudFile(
      uploadForm.localPath.trim(),
      uploadForm.remotePath.trim()
    );
    showMessage(result.message || "上传成功", "success");
    await refreshList();
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "上传失败", "error");
  } finally {
    uploading.value = false;
  }
}

async function downloadFile(item: CloudFile) {
  if (downloadingPath.value) {
    return;
  }

  const remotePath = joinRemotePath(remoteDir.value, item.path);
  const savePath = await save({
    defaultPath: item.path,
  });

  if (typeof savePath !== "string" || !savePath.trim()) {
    return;
  }

  downloadingPath.value = remotePath;

  try {
    const savedPath = await downloadCloudFile(remotePath, savePath);
    showMessage(`已下载到 ${savedPath}`, "success");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "下载失败", "error");
  } finally {
    downloadingPath.value = "";
  }
}

async function removeFile(item: CloudFile) {
  if (deletingPath.value) {
    return;
  }

  const remotePath = joinRemotePath(remoteDir.value, item.path);
  const confirmed = window.confirm(`确认删除云端文件 ${remotePath} 吗？`);

  if (!confirmed) {
    return;
  }

  deletingPath.value = remotePath;

  try {
    const result = await deleteCloudFile(remotePath);
    showMessage(result.message || "删除成功", "success");
    await refreshList();
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "删除失败", "error");
  } finally {
    deletingPath.value = "";
  }
}

function fillRemotePathFromLocal() {
  if (!uploadForm.localPath.trim()) {
    return;
  }

  const fileName = uploadForm.localPath.trim().split(/[/\\]/).pop();

  if (!fileName) {
    return;
  }

  uploadForm.remotePath = joinRemotePath(remoteDir.value, fileName);
}

onMounted(() => {
  refreshList();
});
</script>

<template>
  <section class="panel">
    <h2>云存储</h2>
    <p class="hint">
      浏览、上传、下载和删除云端备份文件。本地 Sidecar 默认使用应用数据目录下的
      <code>cloud_storage</code> 作为存储根目录。
    </p>

    <div class="form-grid">
      <label class="field span-2">
        <span>云存储 Token（可选，对应服务端 BACKUP_CLOUD_TOKEN）</span>
        <div class="path-picker-row">
          <input
            v-model="cloudToken"
            type="password"
            placeholder="远程网盘模式时填写"
          />
          <button type="button" class="secondary path-picker-button" @click="saveToken">
            保存
          </button>
        </div>
      </label>

      <label class="field span-2">
        <span>云端目录</span>
        <div class="path-picker-row">
          <input
            v-model="remoteDir"
            type="text"
            placeholder="留空表示根目录，例如 backups"
          />
          <SubmitButton
            variant="secondary"
            class="path-picker-button"
            :loading="listing"
            @click="refreshList(true)"
          >
            {{ listing ? "刷新中..." : "刷新列表" }}
          </SubmitButton>
        </div>
      </label>
    </div>

    <div class="panel-subsection">
      <h3>上传文件</h3>
      <div class="form-grid">
        <PathPicker
          v-model="uploadForm.localPath"
          class="span-2"
          label="本地文件"
          placeholder="/path/to/local/file"
          mode="file"
          recent-key="cloud-upload-local"
          :disabled="uploading"
          @update:model-value="fillRemotePathFromLocal"
        />
        <label class="field span-2">
          <span>云端保存路径</span>
          <input
            v-model="uploadForm.remotePath"
            type="text"
            placeholder="例如 backups/backup_20260710.zip"
            :disabled="uploading"
          />
        </label>
      </div>
      <div class="actions">
        <SubmitButton :loading="uploading" @click="uploadFile">
          {{ uploading ? "正在上传..." : "上传到云端" }}
        </SubmitButton>
      </div>
    </div>

    <div class="panel-subsection">
      <h3>云端文件列表</h3>

      <EmptyState
        v-if="hasQueried && files.length === 0"
        title="当前目录暂无文件"
        description="可先上传备份文件，或切换到其他云端目录。"
      />

      <div v-else-if="files.length > 0" class="table-wrap">
        <table class="data-table">
          <thead>
            <tr>
              <th>文件名</th>
              <th>大小</th>
              <th>修改时间</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in files" :key="item.path">
              <td class="mono">{{ item.path }}</td>
              <td>{{ formatBytes(item.size) }}</td>
              <td>{{ formatTimestamp(item.modified) }}</td>
              <td class="table-actions">
                <SubmitButton
                  variant="secondary"
                  class="small"
                  :loading="downloadingPath === joinRemotePath(remoteDir, item.path)"
                  :disabled="Boolean(downloadingPath) && downloadingPath !== joinRemotePath(remoteDir, item.path)"
                  @click="downloadFile(item)"
                >
                  {{
                    downloadingPath === joinRemotePath(remoteDir, item.path)
                      ? "下载中..."
                      : "下载"
                  }}
                </SubmitButton>
                <SubmitButton
                  variant="danger"
                  class="small"
                  :loading="deletingPath === joinRemotePath(remoteDir, item.path)"
                  :disabled="Boolean(deletingPath) && deletingPath !== joinRemotePath(remoteDir, item.path)"
                  @click="removeFile(item)"
                >
                  {{
                    deletingPath === joinRemotePath(remoteDir, item.path)
                      ? "删除中..."
                      : "删除"
                  }}
                </SubmitButton>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </section>
</template>
