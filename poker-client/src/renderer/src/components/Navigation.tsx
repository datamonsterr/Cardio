import React, { useState } from 'react'
import { Link, useLocation, useNavigate } from 'react-router-dom'
import { useAuth } from '../contexts/AuthContext'

const Navigation: React.FC = () => {
  const location = useLocation()
  const navigate = useNavigate()
  const { user, logout } = useAuth()
  const [isVisible, setIsVisible] = useState(false)

  if (!user || location.pathname === '/login') return null

  const handleMouseEnter = (): void => setIsVisible(true)
  const handleMouseLeave = (): void => setIsVisible(false)

  const isGamePage = location.pathname === '/game'

  const handleButtonClick = (): void => {
    if (isGamePage) {
      if (confirm('Are you sure you want to quit the game and return to home?')) {
        navigate('/home')
      }
    } else {
      logout()
      navigate('/login')
    }
  }

  return (
    <>
      {/* Hover trigger area at top */}
      <div
        className="nav-trigger"
        onMouseEnter={handleMouseEnter}
        style={{
          position: 'fixed',
          top: 0,
          left: 0,
          right: 0,
          height: '10px',
          zIndex: 999,
          cursor: 'pointer'
        }}
      />

      {/* Navigation Bar */}
      <nav
        className="poker-nav"
        style={{
          transform: isVisible ? 'translateY(0)' : 'translateY(-100%)',
          transition: 'transform 0.3s ease',
          position: 'fixed',
          top: 0,
          left: 0,
          right: 0,
          zIndex: 1000
        }}
        onMouseEnter={handleMouseEnter}
        onMouseLeave={handleMouseLeave}
      >
        <div className="nav-container">
          <div className="nav-brand">
            <Link to="/home">
              <h2>🃏 Poker Online</h2>
            </Link>
          </div>
          <div className="nav-links">
            {/* <Link to="/home" className={location.pathname === '/home' ? 'active' : ''}>
              Home
            </Link>
            <Link to="/tables" className={location.pathname === '/tables' ? 'active' : ''}>
              Tables
            </Link>
            <Link to="/friends" className={location.pathname === '/friends' ? 'active' : ''}>
              Friends
            </Link> */}
          </div>
          <div className="nav-user">
            <span className="user-info">
              💰 ${user.chips} | {user.username}
            </span>
            <button
              onClick={handleButtonClick}
              className={isGamePage ? 'quit-nav-btn' : 'logout-btn'}
            >
              {isGamePage ? 'Quit Room' : 'Logout'}
            </button>
          </div>
        </div>
      </nav>
    </>
  )
}

export default Navigation
