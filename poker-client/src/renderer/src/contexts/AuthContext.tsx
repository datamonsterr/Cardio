import React, { createContext, useContext, useState, useEffect, ReactNode } from 'react';
import type { User, AuthContextType } from '../types';
import { authenticateUser } from '../data/mockUsers';

const AuthContext = createContext<AuthContextType | undefined>(undefined);

interface AuthProviderProps {
  children: ReactNode;
}

export function AuthProvider({ children }: AuthProviderProps): React.JSX.Element {
  const [user, setUser] = useState<User | null>(null);
  const [loading, setLoading] = useState<boolean>(true);

  useEffect(() => {
    // Check for stored user on mount
    const storedUser = localStorage.getItem('pokerUser');
    if (storedUser) {
      try {
        setUser(JSON.parse(storedUser));
      } catch (error) {
        console.error('Failed to parse stored user:', error);
        localStorage.removeItem('pokerUser');
      }
    }
    setLoading(false);
  }, []);

  const login = (username: string, password: string): boolean => {
    const authenticatedUser = authenticateUser(username, password);

    if (authenticatedUser) {
      // Remove password before storing
      const { password: _, ...userWithoutPassword } = authenticatedUser;
      setUser(userWithoutPassword as User);
      localStorage.setItem('pokerUser', JSON.stringify(userWithoutPassword));
      return true;
    }
    return false;
  };

  const logout = (): void => {
    setUser(null);
    localStorage.removeItem('pokerUser');
  };

  const updateChips = (amount: number): void => {
    setUser((prev) => {
      if (!prev) return null;
      const updated = { ...prev, chips: prev.chips + amount };
      localStorage.setItem('pokerUser', JSON.stringify(updated));
      return updated;
    });
  };

  return (
    <AuthContext.Provider value={{ user, login, logout, updateChips, loading }}>
      {children}
    </AuthContext.Provider>
  );
}

export function useAuth(): AuthContextType {
  const context = useContext(AuthContext);
  if (context === undefined) {
    throw new Error('useAuth must be used within an AuthProvider');
  }
  return context;
}
