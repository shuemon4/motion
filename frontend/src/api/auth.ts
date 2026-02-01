import { setSession, clearSession, getSessionToken } from './session';
import { apiGet } from './client';

interface LoginResponse {
  session_token: string;
  csrf_token: string;
  role: 'admin' | 'user';
  expires_in: number;
}

interface AuthStatusResponse {
  auth_required: boolean;
  authenticated: boolean;
  role?: 'admin' | 'user';
  csrf_token?: string;
  session_token?: string;
}

/**
 * Login with username and password
 */
export async function login(
  username: string,
  password: string,
  rememberMe: boolean = false
): Promise<{ success: boolean; error?: string; role?: string }> {
  try {
    const response = await fetch('/0/api/auth/login', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify({ username, password }),
    });

    if (response.ok) {
      const data: LoginResponse = await response.json();
      setSession(data, rememberMe);
      return { success: true, role: data.role };
    }

    if (response.status === 401) {
      return { success: false, error: 'Invalid username or password' };
    }

    if (response.status === 429) {
      return { success: false, error: 'Too many attempts. Please wait.' };
    }

    return { success: false, error: `Login failed (${response.status})` };
  } catch {
    return { success: false, error: 'Connection failed. Please try again.' };
  }
}

/**
 * Logout - destroy session
 */
export async function logout(): Promise<void> {
  const token = getSessionToken();

  if (token) {
    try {
      await fetch('/0/api/auth/logout', {
        method: 'POST',
        headers: {
          'X-Session-Token': token,
        },
      });
    } catch {
      // Ignore errors - clear local session anyway
    }
  }

  clearSession();
}

/**
 * Check current authentication status.
 * Uses apiGet so session token is sent consistently and backend sliding window is extended.
 */
export async function getAuthStatus(): Promise<AuthStatusResponse> {
  try {
    const data = await apiGet<AuthStatusResponse>('/0/api/auth/status');
    if (data.session_token && data.csrf_token && data.role) {
      setSession({
        session_token: data.session_token,
        csrf_token: data.csrf_token,
        role: data.role as 'admin' | 'user',
      }, false);
    }
    return data;
  } catch {
    // Auth status endpoint should not 401 (it returns authenticated:false instead)
    // But if it does (server error), return safe default
    return { auth_required: true, authenticated: false };
  }
}
