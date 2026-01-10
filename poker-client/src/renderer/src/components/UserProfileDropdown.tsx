import React, { useState, useRef, useEffect } from 'react'
import { useAuth } from '../contexts/AuthContext'
import '../assets/user-profile.css'

interface UserProfileDropdownProps {
  onLogout?: () => void
}

export const UserProfileDropdown: React.FC<UserProfileDropdownProps> = ({ onLogout }) => {
  const { user } = useAuth()
  const [isOpen, setIsOpen] = useState(false)
  const dropdownRef = useRef<HTMLDivElement>(null)

  // Close dropdown when clicking outside
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
        setIsOpen(false)
      }
    }

    if (isOpen) {
      document.addEventListener('mousedown', handleClickOutside)
    }

    return () => {
      document.removeEventListener('mousedown', handleClickOutside)
    }
  }, [isOpen])

  if (!user) return null

  const toggleDropdown = () => setIsOpen(!isOpen)

  return (
    <div className="user-profile-dropdown" ref={dropdownRef}>
      <button className="profile-trigger" onClick={toggleDropdown}>
        <div className="profile-avatar">{user.username.charAt(0).toUpperCase()}</div>
        <div className="profile-info">
          <span className="profile-username">{user.username}</span>
          <span className="profile-chips">${user.chips.toLocaleString()}</span>
        </div>
        <svg
          className={`dropdown-arrow ${isOpen ? 'open' : ''}`}
          width="12"
          height="12"
          viewBox="0 0 12 12"
          fill="currentColor"
        >
          <path d="M2 4l4 4 4-4" stroke="currentColor" strokeWidth="2" fill="none" />
        </svg>
      </button>

      {isOpen && (
        <div className="profile-dropdown-menu">
          <div className="profile-header">
            <div className="profile-avatar-large">{user.username.charAt(0).toUpperCase()}</div>
            <div className="profile-details">
              <h3>{user.username}</h3>
              <p className="user-id">ID: {user.id}</p>
            </div>
          </div>

          <div className="profile-stats">
            <div className="stat-item">
              <span className="stat-label">Balance</span>
              <span className="stat-value">${user.chips.toLocaleString()}</span>
            </div>
            {user.email && (
              <div className="stat-item">
                <span className="stat-label">Email</span>
                <span className="stat-value">{user.email}</span>
              </div>
            )}
            <div className="stat-item">
              <span className="stat-label">Games Played</span>
              <span className="stat-value">{user.gamesPlayed || 0}</span>
            </div>
            <div className="stat-item">
              <span className="stat-label">Wins</span>
              <span className="stat-value">{user.wins || 0}</span>
            </div>
            {user.totalWinnings !== undefined && (
              <div className="stat-item">
                <span className="stat-label">Total Winnings</span>
                <span className="stat-value profit">${user.totalWinnings.toLocaleString()}</span>
              </div>
            )}
            {user.level !== undefined && (
              <div className="stat-item">
                <span className="stat-label">Level</span>
                <span className="stat-value">{user.level}</span>
              </div>
            )}
          </div>

          {onLogout && (
            <div className="profile-actions">
              <button className="logout-action-btn" onClick={onLogout}>
                <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                  <path d="M10 3.5a.5.5 0 0 0-.5-.5h-8a.5.5 0 0 0-.5.5v9a.5.5 0 0 0 .5.5h8a.5.5 0 0 0 .5-.5v-2a.5.5 0 0 1 1 0v2A1.5 1.5 0 0 1 9.5 14h-8A1.5 1.5 0 0 1 0 12.5v-9A1.5 1.5 0 0 1 1.5 2h8A1.5 1.5 0 0 1 11 3.5v2a.5.5 0 0 1-1 0v-2z" />
                  <path d="M4.146 8.354a.5.5 0 0 1 0-.708l3-3a.5.5 0 1 1 .708.708L5.707 7.5H14.5a.5.5 0 0 1 0 1H5.707l2.147 2.146a.5.5 0 0 1-.708.708l-3-3z" />
                </svg>
                Logout
              </button>
            </div>
          )}
        </div>
      )}
    </div>
  )
}
