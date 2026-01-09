import React, { createContext, useContext, useState, useEffect, ReactNode } from 'react';
import type { User, AuthContextType } from '../types';
import { AuthService, ConnectionStatus } from '../services/auth/AuthService';
import { Packet, SignupRequest } from '../services/protocol';

// Global connection status state
let globalConnectionStatus: ConnectionStatus = 'disconnected';
let globalConnectionMessage: string = '';
const connectionStatusListeners: Set<(status: ConnectionStatus, message: string) => void> = new Set();

// Create auth service instance
const authService = new AuthService({
  host: 'localhost',
  port: 8080,
  onDisconnect: () => {
    console.log('Disconnected from server');
  },
  onPacket: (packet: Packet) => {
    console.log('Received packet:', packet.header.packet_type);
  },
  onConnectionStatusChange: (status: ConnectionStatus, message?: string) => {
    console.log('Connection status:', status, message);
    globalConnectionStatus = status;
    globalConnectionMessage = message || '';
    connectionStatusListeners.forEach(listener => listener(status, message || ''));
  }
});

export const subscribeToConnectionStatus = (
  listener: (status: ConnectionStatus, message: string) => void
): (() => void) => {
  connectionStatusListeners.add(listener);
  // Immediately call with current status
  listener(globalConnectionStatus, globalConnectionMessage);
  return () => connectionStatusListeners.delete(listener);
};

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

    // Connect to server immediately on app start
    const connectToServer = async () => {
      try {
        await authService.connect();
        console.log('Connected to server on app start');
      } catch (error) {
        console.error('Failed to connect to server on app start:', error);
      }
    };

    connectToServer();
  }, []);

  const login = async (username: string, password: string): Promise<boolean> => {
    try {
      const authenticatedUser = await authService.login(username, password);

      // Convert AuthUser to our User type and store
      const user: User = {
        id: authenticatedUser.user_id.toString(),
        username: authenticatedUser.username,
        email: authenticatedUser.email,
        chips: authenticatedUser.balance,
        avatarURL: authenticatedUser.avatar_url,
        gamesPlayed: 0,
        wins: 0,
        totalWinnings: 0
      };

      setUser(user);
      localStorage.setItem('pokerUser', JSON.stringify(user));
      return true;
    } catch (error) {
      console.error('Login failed:', error);
      return false;
    }
  };

  const logout = (): void => {
    authService.logout();
    setUser(null);
    localStorage.removeItem('pokerUser');
  };

  const signup = async (request: SignupRequest): Promise<void> => {
    await authService.signup(request);
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
    <AuthContext.Provider value={{ user, login, logout, updateChips, loading, signup }}>
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
