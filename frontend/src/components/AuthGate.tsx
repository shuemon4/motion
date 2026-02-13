import { type ReactNode, useState } from 'react';
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
  const [wasAuthenticated, setWasAuthenticated] = useState(false);

  // Adjust state during render (React-recommended pattern for derived state)
  if (isAuthenticated && !wasAuthenticated) {
    setWasAuthenticated(true);
  }

  const sessionExpired = wasAuthenticated && authRequired && !isAuthenticated;

  const handleLoginSuccess = () => {
    setWasAuthenticated(true);
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
