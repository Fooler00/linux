<script setup lang="ts">
// 轻量空态组件，统一“暂无数据 / 正在加载”类提示，避免每个页面各写一套占位块。
withDefaults(
  defineProps<{
    title: string;
    description?: string;
  }>(),
  {
    description: "",
  }
);
</script>

<template>
  <div class="empty-state" role="status" aria-live="polite">
    <p class="empty-state-title">{{ title }}</p>
    <p v-if="description" class="empty-state-description">{{ description }}</p>
  </div>
</template>
