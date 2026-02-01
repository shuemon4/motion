import { type ReactNode, useRef } from 'react';
import { useQueryClient } from '@tanstack/react-query';
import { useNavigate } from 'react-router-dom';
import { restoreSession } from '@/api/session';
import { useAuthContext } from '@/contexts/AuthContext';
import { LoginPage } from './LoginPage';

// Restore session synchronously on module load
restoreSession();

interface AuthGateProps {
  children: ReactNode;
}

export function AuthGate({ children }: AuthGateProps) {
  const queryClient = useQueryClient();
  const navigate = useNavigate();
  const { isAuthenticated, authRequired, isLoading } = useAuthContext();
  const wasAuthenticated = useRef(false);

  if (isAuthenticated) wasAuthenticated.current = true;
  const sessionExpired = wasAuthenticated.current && authRequired && !isAuthenticated;

  const handleLoginSuccess = () => {
    wasAuthenticated.current = true;
    queryClient.invalidateQueries({ queryKey: ['auth'] });
    navigate('/');
  };

  if (isLoading) {
    return (
      <div className="min-h-screen bg-surface flex items-center justify-center">
        <div className="animate-pulse text-text-secondary">Loading...</div>
      </div>
    );
  }

  if (!authRequired) return <>{children}</>;

  if (!isAuthenticated) {
    return <LoginPage onSuccess={handleLoginSuccess} sessionExpired={sessionExpired} />;
  }

  return <>{children}</>;
}
