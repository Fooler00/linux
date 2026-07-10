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
    <!-- 轻量装饰图标，增强空态辨识度而不喧宾夺主 -->
    <div class="empty-state-icon" aria-hidden="true">
      <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
        <rect x="3" y="3" width="18" height="18" rx="2" ry="2"/>
        <line x1="9" y1="9" x2="15" y2="15"/>
        <line x1="15" y1="9" x2="9" y2="15"/>
      </svg>
    </div>
    <p class="empty-state-title">{{ title }}</p>
    <p v-if="description" class="empty-state-description">{{ description }}</p>
  </div>
</template>
