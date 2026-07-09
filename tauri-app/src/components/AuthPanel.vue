<script setup lang="ts">
import { computed, reactive, ref } from "vue";
import { login, register } from "../api/auth";
import type {
  AuthResponse,
  LoginRequest,
  RegisterRequest,
} from "../api/types";

const emit = defineEmits<{
  authenticated: [user: AuthResponse];
}>();

const mode = ref<"login" | "register">("login");
const loading = ref(false);
const errorMessage = ref("");
const form = reactive({
  username: "",
  password: "",
});

const title = computed(() =>
  mode.value === "login" ? "登录你的账号" : "创建新账号"
);

const submitLabel = computed(() =>
  loading.value
    ? mode.value === "login"
      ? "登录中..."
      : "注册中..."
    : mode.value === "login"
      ? "登录"
      : "注册并进入"
);

const switchLabel = computed(() =>
  mode.value === "login" ? "没有账号？去注册" : "已有账号？去登录"
);

function switchMode(nextMode: "login" | "register") {
  mode.value = nextMode;
  errorMessage.value = "";
}

async function submit() {
  const username = form.username.trim();
  const password = form.password;

  if (!username) {
    errorMessage.value = "请输入用户名";
    return;
  }

  if (!password) {
    errorMessage.value = "请输入密码";
    return;
  }

  loading.value = true;
  errorMessage.value = "";

  try {
    const payload: LoginRequest | RegisterRequest = { username, password };
    const user =
      mode.value === "login" ? await login(payload) : await register(payload);
    form.username = username;
    emit("authenticated", user);
  } catch (error) {
    errorMessage.value =
      error instanceof Error ? error.message : "认证失败，请稍后重试";
  } finally {
    loading.value = false;
  }
}
</script>

<template>
  <section class="auth-layout">
    <div class="auth-card panel">
      <div class="auth-tabs">
        <button
          type="button"
          class="tab"
          :class="{ active: mode === 'login' }"
          :disabled="loading"
          @click="switchMode('login')"
        >
          登录
        </button>
        <button
          type="button"
          class="tab"
          :class="{ active: mode === 'register' }"
          :disabled="loading"
          @click="switchMode('register')"
        >
          注册
        </button>
      </div>

      <h2>{{ title }}</h2>
      <p class="hint auth-hint">登录后即可进入当前备份工作台。</p>

      <form class="form-grid auth-form" @submit.prevent="submit">
        <label class="field span-2">
          <span>用户名</span>
          <input
            v-model="form.username"
            type="text"
            autocomplete="username"
            maxlength="64"
            :disabled="loading"
            placeholder="请输入用户名"
          />
        </label>

        <label class="field span-2">
          <span>密码</span>
          <input
            v-model="form.password"
            type="password"
            autocomplete="current-password"
            maxlength="128"
            :disabled="loading"
            placeholder="请输入密码"
          />
        </label>

        <p v-if="errorMessage" class="auth-error">{{ errorMessage }}</p>

        <div class="actions span-2 auth-actions">
          <button type="submit" class="primary" :disabled="loading">
            <span v-if="loading" class="button-spinner" aria-hidden="true"></span>
            {{ submitLabel }}
          </button>
          <button
            type="button"
            class="secondary"
            :disabled="loading"
            @click="switchMode(mode === 'login' ? 'register' : 'login')"
          >
            {{ switchLabel }}
          </button>
        </div>
      </form>
    </div>
  </section>
</template>
