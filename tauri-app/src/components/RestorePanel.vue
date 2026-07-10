<script setup lang="ts">
import { reactive, ref } from "vue";
import { startRestore } from "../api/backup";
import { showMessage } from "../composables/useMessage";
import PathPicker from "./PathPicker.vue";
import SubmitButton from "./SubmitButton.vue";
import { ENCRYPT_ALGO_OPTIONS } from "../utils/options";
import { validateRestoreForm } from "../utils/validation";

const emit = defineEmits<{ submitted: [] }>();
// 防止同一份还原表单被连续提交，重复创建真实还原任务。
const submitting = ref(false);

const form = reactive({
  backupPath: "",
  destination: "",
  password: "",
  encryptAlgo: "aes-256-cbc",
});

async function submit() {
  if (submitting.value) {
    return;
  }

  const validation = validateRestoreForm(form);
  if (!validation.valid) {
    showMessage(validation.message ?? "请检查还原表单", "error");
    return;
  }

  submitting.value = true;

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
  } finally {
    submitting.value = false;
  }
}
</script>

<template>
  <section class="panel">
    <h2>还原备份</h2>
    <p class="hint">指定备份文件路径与还原目标目录，加密备份需填写对应密码。</p>
    <div class="form-grid">
      <PathPicker
        v-model="form.backupPath"
        class="span-2"
        label="备份文件/目录路径"
        placeholder="/path/to/backup_xxx"
        mode="file"
        recent-key="restore-files"
        :disabled="submitting"
      />
      <PathPicker
        v-model="form.destination"
        class="span-2"
        label="还原目标目录"
        placeholder="/path/to/restore"
        mode="directory"
        recent-key="restore-destinations"
        :disabled="submitting"
      />
      <label class="field">
        <span>解密密码（加密备份需要）</span>
        <input v-model="form.password" type="password" />
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
    </div>
    <div class="actions">
      <SubmitButton :loading="submitting" @click="submit">
        {{ submitting ? "正在提交还原任务..." : "开始还原" }}
      </SubmitButton>
    </div>
  </section>
</template>
