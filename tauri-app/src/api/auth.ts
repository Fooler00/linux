import { apiRequestOrThrow } from "./client";
import type {
  AuthResponse,
  LoginRequest,
  RegisterRequest,
} from "./types";

export function login(payload: LoginRequest) {
  return apiRequestOrThrow<AuthResponse>("POST", "/api/login", payload);
}

export function register(payload: RegisterRequest) {
  return apiRequestOrThrow<AuthResponse>("POST", "/api/register", payload);
}
