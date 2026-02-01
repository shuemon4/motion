/**
 * Authentication Context for React UI
 *
 * Provides reactive authentication state management for the application.
 * Uses TanStack Query to automatically update when auth state changes.
 * Backend is the single source of truth for session validity.
 */

import {
  createContext,
  useContext,
  type ReactNode,
} from 'react'
import { useQuery } from '@tanstack/react-query'
import { getAuthStatus } from '@/api/auth'

interface AuthContextValue {
  /** Whether user is currently authenticated */
  isAuthenticated: boolean
  /** User role (admin or user) or null if not authenticated */
  role: 'admin' | 'user' | null
  /** Whether auth status is still loading */
  isLoading: boolean
  /** Whether authentication is required (configured in Motion) */
  authRequired: boolean
}

const AuthContext = createContext<AuthContextValue | null>(null)

interface AuthProviderProps {
  children: ReactNode
}

export function AuthProvider({ children }: AuthProviderProps) {
  const { data: authStatus, isLoading } = useQuery({
    queryKey: ['auth', 'status'],
    queryFn: getAuthStatus,
    staleTime: 5000,
    refetchInterval: 120000,
    refetchIntervalInBackground: false,
    retry: 1,
  })

  const value: AuthContextValue = {
    isAuthenticated: authStatus?.authenticated ?? false,
    role: authStatus?.role ?? null,
    isLoading,
    authRequired: authStatus?.auth_required ?? true,
  }

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

/**
 * Hook to access authentication context
 * @throws Error if used outside of AuthProvider
 */
export function useAuthContext(): AuthContextValue {
  const context = useContext(AuthContext)
  if (!context) {
    throw new Error('useAuthContext must be used within an AuthProvider')
  }
  return context
}
