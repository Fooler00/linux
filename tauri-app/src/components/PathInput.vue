<script setup lang="ts">
const model = defineModel<string>({ required: true });

withDefaults(
  defineProps<{
    label: string;
    placeholder?: string;
    disabled?: boolean;
  }>(),
  {
    placeholder: "",
    disabled: false,
  }
);
</script>

<template>
  <label class="field path-input">
    <span>{{ label }}</span>
    <input v-model="model" type="text" :placeholder="placeholder" :disabled="disabled" />
    <!-- TODO: 当前阶段只保留文本路径输入；项目未启用 Tauri dialog 插件和权限，后续确认后再接入系统路径选择。 -->
  </label>
</template>
