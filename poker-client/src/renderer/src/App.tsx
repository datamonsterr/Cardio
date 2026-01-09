import React, { useState, useEffect } from 'react'
import { BrowserRouter as Router, Routes, Route, Navigate, useLocation } from 'react-router-dom'
import { AuthProvider, useAuth, subscribeToConnectionStatus } from './contexts/AuthContext'
import Navigation from './components/Navigation'
import ConnectionStatus from './components/ConnectionStatus'
import LoginPage from './pages/LoginPage'
import SignupPage from './pages/SignupPage'
import HomePage from './pages/HomePage'
import TablesPage from './pages/TablesPage'
import GamePage from './pages/GamePage'
import { FriendsPage } from './pages/FriendsPage'
import type { ConnectionStatusType } from './components/ConnectionStatus'

// Import CSS
import './assets/app.css'
import './assets/globals.css'
import './assets/index.css'
import './assets/navigation.css'
import './assets/login.css'
import './assets/home.css'
import './assets/tables.css'
import './assets/App.css'
import './assets/Poker.css'

// Protected Route Component
const ProtectedRoute: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const { user, loading } = useAuth()
  const location = useLocation()

  if (loading) {
    return (
      <div className="loading-container">
        <div className="spinner"></div>
      </div>
    )
  }

  if (!user) {
    return <Navigate to="/login" state={{ from: location }} replace />
  }

  return <>{children}</>
}

// Public Route (redirect to home if already logged in)
const PublicRoute: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const { user, loading } = useAuth()

  if (loading) {
    return (
      <div className="loading-container">
        <div className="spinner"></div>
      </div>
    )
  }

  if (user) {
    return <Navigate to="/home" replace />
  }

  return <>{children}</>
}

function AppContent(): React.JSX.Element {
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatusType>('disconnected')
  const [connectionMessage, setConnectionMessage] = useState<string>('')

  useEffect(() => {
    const unsubscribe = subscribeToConnectionStatus((status, message) => {
      // Map internal status to component status
      const statusMap: Record<string, ConnectionStatusType> = {
        disconnected: 'disconnected',
        connecting: 'connecting',
        handshaking: 'connecting',
        connected: 'connected',
        error: 'disconnected'
      }
      setConnectionStatus(statusMap[status] || 'disconnected')
      setConnectionMessage(message)
    })

    return unsubscribe
  }, [])

  return (
    <div className="app-container">
      <ConnectionStatus status={connectionStatus} message={connectionMessage} />
      <Navigation />
      <Routes>
        <Route
          path="/login"
          element={
            <PublicRoute>
              <LoginPage />
            </PublicRoute>
          }
        />
        <Route
          path="/signup"
          element={
            <PublicRoute>
              <SignupPage />
            </PublicRoute>
          }
        />
        <Route
          path="/home"
          element={
            <ProtectedRoute>
              <HomePage />
            </ProtectedRoute>
          }
        />
        <Route
          path="/tables"
          element={
            <ProtectedRoute>
              <TablesPage />
            </ProtectedRoute>
          }
        />
        <Route
          path="/game"
          element={
            <ProtectedRoute>
              <GamePage />
            </ProtectedRoute>
          }
        />
        <Route
          path="/friends"
          element={
            <ProtectedRoute>
              <FriendsPage />
            </ProtectedRoute>
          }
        />
        <Route path="/" element={<Navigate to="/login" replace />} />
        <Route path="*" element={<Navigate to="/login" replace />} />
      </Routes>
    </div>
  )
}

function App(): React.JSX.Element {
  return (
    <Router>
      <AuthProvider>
        <AppContent />
      </AuthProvider>
    </Router>
  )
}

export default App
