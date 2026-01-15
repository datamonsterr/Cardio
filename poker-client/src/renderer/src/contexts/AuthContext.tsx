import React, { createContext, useContext, useState, useEffect, ReactNode } from 'react'
import type { User, AuthContextType, CreateTableRequestType } from '../types'
import { AuthService, ConnectionStatus } from '../services/auth/AuthService'
import { HeartbeatService } from '../services/network/HeartbeatService'
import { Packet, SignupRequest, CreateTableRequest, PACKET_TYPE, PACKET_BALANCE_UPDATE, decodeBalanceUpdateNotification } from '../services/protocol'
import { getServerConfig } from '../config/server'

// Global connection status state
let globalConnectionStatus: ConnectionStatus = 'disconnected'
let globalConnectionMessage: string = ''
const connectionStatusListeners: Set<(status: ConnectionStatus, message: string) => void> =
  new Set()

// Global user update management
type UserUpdater = (updater: (prev: User | null) => User | null) => void
let globalUserUpdater: UserUpdater | null = null

// Create heartbeat service instance
const heartbeatService = new HeartbeatService({
  onStatusChange: (status, message) => {
    // Map heartbeat status to connection status
    const connectionStatus: ConnectionStatus =
      status === 'connected'
        ? 'connected'
        : status === 'reconnecting'
          ? 'connecting'
          : 'disconnected'

    console.log('Heartbeat status:', status, message)
    globalConnectionStatus = connectionStatus
    globalConnectionMessage = message || ''
    connectionStatusListeners.forEach((listener) => listener(connectionStatus, message || ''))
  }
})

// Create auth service instance
const serverConfig = getServerConfig()
const authService = new AuthService({
  host: serverConfig.host,
  port: serverConfig.port,
  onDisconnect: () => {
    console.log('Disconnected from server')
    heartbeatService.stop()
  },
  onPacket: (packet: Packet) => {
    // Handle PONG for heartbeat
    if (packet.header.packet_type === PACKET_TYPE.PONG) {
      heartbeatService.onPongReceived()
      return
    }
    
    // Handle balance updates
    if (packet.header.packet_type === PACKET_BALANCE_UPDATE) {
      try {
        const balanceUpdate = decodeBalanceUpdateNotification(packet.data)
        console.log('Balance update received:', balanceUpdate)
        // Update user balance in context using global updater
        if (globalUserUpdater) {
          globalUserUpdater((prev) => {
            if (!prev) return null
            const updated = { ...prev, chips: balanceUpdate.balance }
            localStorage.setItem('pokerUser', JSON.stringify(updated))
            return updated
          })
        }
      } catch (error) {
        console.error('Failed to decode balance update:', error)
      }
      return
    }
    
    console.log('Received packet:', packet.header.packet_type)
  },
  onConnectionStatusChange: (status: ConnectionStatus, message?: string) => {
    console.log('Connection status:', status, message)
    // Update global status for non-heartbeat connection events
    globalConnectionStatus = status
    globalConnectionMessage = message || ''
    connectionStatusListeners.forEach((listener) => listener(status, message || ''))

    // Start/stop heartbeat based on connection status
    if (status === 'connected') {
      // Start heartbeat when connection is established
      const client = authService.getClient()
      if (client) {
        heartbeatService.start(client)
      }
    } else if (status === 'disconnected' || status === 'error') {
      heartbeatService.stop()
    }
  }
})

export const subscribeToConnectionStatus = (
  listener: (status: ConnectionStatus, message: string) => void
): (() => void) => {
  connectionStatusListeners.add(listener)
  // Immediately call with current status
  listener(globalConnectionStatus, globalConnectionMessage)
  return () => connectionStatusListeners.delete(listener)
}

const AuthContext = createContext<AuthContextType | undefined>(undefined)

interface AuthProviderProps {
  children: ReactNode
}

export function AuthProvider({ children }: AuthProviderProps): React.JSX.Element {
  const [user, setUser] = useState<User | null>(null)
  const [loading, setLoading] = useState<boolean>(true)

  // Register global user updater
  useEffect(() => {
    globalUserUpdater = setUser
    return () => {
      globalUserUpdater = null
    }
  }, [setUser])

  useEffect(() => {
    // Check for stored user on mount
    const storedUser = localStorage.getItem('pokerUser')
    if (storedUser) {
      try {
        setUser(JSON.parse(storedUser))
      } catch (error) {
        console.error('Failed to parse stored user:', error)
        localStorage.removeItem('pokerUser')
      }
    }
    setLoading(false)

    // Connect to server immediately on app start
    const connectToServer = async () => {
      try {
        await authService.connect()
        console.log('Connected to server on app start')
      } catch (error) {
        console.error('Failed to connect to server on app start:', error)
      }
    }

    connectToServer()
  }, [])

  const login = async (username: string, password: string): Promise<boolean> => {
    try {
      const authenticatedUser = await authService.login(username, password)

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
      }

      setUser(user)
      localStorage.setItem('pokerUser', JSON.stringify(user))
      return true
    } catch (error) {
      console.error('Login failed:', error)
      return false
    }
  }

  const logout = (): void => {
    heartbeatService.stop()
    authService.logout()
    setUser(null)
    localStorage.removeItem('pokerUser')
  }

  const signup = async (request: SignupRequest): Promise<void> => {
    await authService.signup(request)
  }

  const updateChips = (amount: number): void => {
    setUser((prev) => {
      if (!prev) return null
      const updated = { ...prev, chips: prev.chips + amount }
      localStorage.setItem('pokerUser', JSON.stringify(updated))
      return updated
    })
  }
  


  const getTables = async () => {
    try {
      const response = await authService.getTables()
      return response
    } catch (error) {
      console.error('Failed to get tables:', error)
      throw error
    }
  }

  const createTable = async (request: CreateTableRequestType): Promise<number> => {
    try {
      const createRequest: CreateTableRequest = {
        name: request.name,
        max_player: request.max_player,
        min_bet: request.min_bet
      }
      const table_id = await authService.createTable(createRequest)
      return table_id
    } catch (error) {
      console.error('Failed to create table:', error)
      throw error
    }
  }

  const getClient = () => {
    return authService.getClient()
  }

  return (
    <AuthContext.Provider
      value={{
        user,
        login,
        logout,
        updateChips,
        loading,
        signup,
        getTables,
        createTable,
        getClient
      }}
    >
      {children}
    </AuthContext.Provider>
  )
}

export function useAuth(): AuthContextType {
  const context = useContext(AuthContext)
  if (context === undefined) {
    throw new Error('useAuth must be used within an AuthProvider')
  }
  return context
}
