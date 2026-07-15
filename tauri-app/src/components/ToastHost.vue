<script setup lang="ts">
import { dismissToast, messageState } from "../composables/useMessage";
</script>

<template>
  <!-- 右上角浮层 Toast：不占用内容区布局 -->
  <div class="toast-host" aria-live="polite" aria-relevant="additions">
    <div
      v-for="toast in messageState.toasts"
      :key="toast.id"
      class="toast"
      :class="toast.type"
      role="status"
      @click="dismissToast(toast.id)"
    >
      {{ toast.text }}
    </div>
  </div>
</template>
