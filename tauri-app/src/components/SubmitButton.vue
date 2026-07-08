<script setup lang="ts">
withDefaults(
  defineProps<{
    loading?: boolean;
    disabled?: boolean;
    variant?: "primary" | "secondary" | "danger";
  }>(),
  {
    loading: false,
    disabled: false,
    variant: "primary",
  }
);

defineEmits<{
  click: [event: MouseEvent];
}>();
</script>

<template>
  <button
    type="button"
    :class="[variant, { loading }]"
    :disabled="disabled || loading"
    @click="$emit('click', $event)"
  >
    <span v-if="loading" class="button-spinner" aria-hidden="true"></span>
    <span><slot /></span>
  </button>
</template>
