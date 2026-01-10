/**
 * Authentication Context for React UI
 *
 * Provides authentication state management for the application.
 * Uses session-based authentication with server-side token storage.
 */

import {
  createContext,
  useContext,
  type ReactNode,
} from 'react'
import { isAuthenticated, getRole } from '@/api/session'

interface AuthContextValue {
  /** Whether user is currently authenticated */
  isAuthenticated: boolean
  /** User role (admin or user) or null if not authenticated */
  role: 'admin' | 'user' | null
}

const AuthContext = createContext<AuthContextValue | null>(null)

interface AuthProviderProps {
  children: ReactNode
}

export function AuthProvider({ children }: AuthProviderProps) {
  const authenticated = isAuthenticated()
  const role = getRole()

  const value: AuthContextValue = {
    isAuthenticated: authenticated,
    role,
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
