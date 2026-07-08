<script setup lang="ts">
import { reactive } from "vue";
import { startRestore } from "../api/backup";
import { showMessage } from "../composables/useMessage";

const emit = defineEmits<{ submitted: [] }>();

const form = reactive({
  backupPath: "",
  destination: "",
  password: "",
  encryptAlgo: "aes-256-cbc",
});

const encryptAlgos = [
  "aes-256-cbc",
  "aes-128-cbc",
  "camellia-256-cbc",
  "camellia-128-cbc",
  "des-ede3-cbc",
  "chacha20",
];

async function submit() {
  if (!form.backupPath.trim() || !form.destination.trim()) {
    showMessage("请输入备份路径和还原目标目录", "error");
    return;
  }

  try {
    const result = await startRestore({
      backupPath: form.backupPath.trim(),
      destination: form.destination.trim(),
      password: form.password,
      encryptAlgo: form.encryptAlgo,
    });
    showMessage(`还原任务已创建，任务编号：${result.taskId}`, "success");
    emit("submitted");
  } catch (error) {
    showMessage(error instanceof Error ? error.message : "还原失败", "error");
  }
}
</script>

<template>
  <section class="panel">
    <h2>还原备份</h2>
    <div class="form-grid">
      <label class="field span-2">
        <span>备份文件/目录路径</span>
        <input v-model="form.backupPath" type="text" placeholder="/path/to/backup_xxx" />
      </label>
      <label class="field span-2">
        <span>还原目标目录</span>
        <input v-model="form.destination" type="text" placeholder="/path/to/restore" />
      </label>
      <label class="field">
        <span>解密密码（加密备份需要）</span>
        <input v-model="form.password" type="password" />
      </label>
      <label class="field">
        <span>加密算法</span>
        <select v-model="form.encryptAlgo">
          <option v-for="algo in encryptAlgos" :key="algo" :value="algo">{{ algo }}</option>
        </select>
      </label>
    </div>
    <div class="actions">
      <button type="button" class="primary" @click="submit">开始还原</button>
    </div>
  </section>
</template>
