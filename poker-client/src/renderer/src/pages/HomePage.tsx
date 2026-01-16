import React, { useState, useEffect } from 'react'
import { useAuth } from '../contexts/AuthContext'
import { useNavigate } from 'react-router-dom'
import { TablesIcon, ScoreboardIcon, FriendsIcon, PlayNowIcon } from '../components/icons/HomeIcons'
import TableInviteNotification, { TableInvite } from '../components/TableInviteNotification'

const HomePage: React.FC = () => {
  const { user, logout, refreshBalance, tableInvites = [], removeTableInvite } = useAuth()
  const navigate = useNavigate()
  const [activeSection, setActiveSection] = useState<string | null>(null)
  const [isRefreshing, setIsRefreshing] = useState(false)

  if (!user) return null

  // Refresh balance when component mounts to ensure it's current
  useEffect(() => {
    const refreshBalanceOnMount = async () => {
      if (!user || isRefreshing) return

      setIsRefreshing(true)
      try {
        await refreshBalance()
        console.log('HomePage: Balance refresh completed')
      } catch (error) {
        console.error('Failed to refresh balance:', error)
      } finally {
        setIsRefreshing(false)
      }
    }

    // Refresh balance after a short delay to allow any pending balance updates to be processed
    const timer = setTimeout(refreshBalanceOnMount, 500)
    return () => clearTimeout(timer)
  }, [user?.id, refreshBalance, isRefreshing])

  const closeModal = (): void => setActiveSection(null)

  const handlePlayNow = (): void => {
    navigate('/game')
  }

  const handleViewTables = (): void => {
    navigate('/tables')
  }

  const handleViewFriends = (): void => {
    navigate('/friends')
  }

  const handleLogout = (): void => {
    logout()
    navigate('/login')
  }

  const handleReturn = (): void => {
    navigate(-1) // Go back in browser history
  }

  const handleRefreshBalance = async (): Promise<void> => {
    setIsRefreshing(true)
    try {
      await refreshBalance()
    } catch (error) {
      console.error('Failed to refresh balance:', error)
    } finally {
      setIsRefreshing(false)
    }
  }

  const handleAcceptTableInvite = (invite: TableInvite): void => {
    console.log('[HomePage] Accepting table invite:', invite)
    navigate(`/game?tableId=${invite.tableId}`)
    if (removeTableInvite) {
      removeTableInvite(invite.id)
    }
  }

  const handleRejectTableInvite = (invite: TableInvite): void => {
    console.log('[HomePage] Rejecting table invite:', invite)
    if (removeTableInvite) {
      removeTableInvite(invite.id)
    }
  }

  const handleDismissTableInvite = (inviteId: string): void => {
    if (removeTableInvite) {
      removeTableInvite(inviteId)
    }
  }

  return (
    <div className="home-container">
      <div className="home-header">
        <div className="header-content">
          <div className="header-text">
            <h1>Welcome back, {user.username}! </h1>
            <p className="balance-info">
              Balance: ${user.chips.toLocaleString()}
              {isRefreshing && <span className="refreshing"> (refreshing...)</span>}
            </p>
          </div>
          <div className="header-actions">
            <button 
              className="refresh-button" 
              onClick={handleRefreshBalance} 
              title="Refresh balance"
              disabled={isRefreshing}
            >
              {isRefreshing ? 'Refreshing...' : 'Refresh Balance'}
            </button>
            <button className="return-button" onClick={handleReturn} title="Go back">
              ← Return
            </button>
            <button className="logout-button" onClick={handleLogout} title="Logout">
              Logout
            </button>
          </div>
        </div>
      </div>

      <div className="main-buttons-grid">
        <button className="main-button" onClick={handleViewTables}>
          <div className="button-icon">
            <TablesIcon size={64} />
          </div>
          <div className="button-text">
            <h2>Tables</h2>
            <p>Browse & Join Tables</p>
          </div>
        </button>

        <button className="main-button play-now" onClick={handlePlayNow}>
          <div className="button-icon">
            <PlayNowIcon size={64} />
          </div>
          <div className="button-text">
            <h2>Play Now</h2>
            <p>Start a Quick Game</p>
          </div>
        </button>

        <button className="main-button" onClick={() => setActiveSection('scoreboard')}>
          <div className="button-icon">
            <ScoreboardIcon size={64} />
          </div>
          <div className="button-text">
            <h2>Scoreboard</h2>
            <p>View Rankings</p>
          </div>
        </button>

        <button className="main-button" onClick={handleViewFriends}>
          <div className="button-icon">
            <FriendsIcon size={64} />
          </div>
          <div className="button-text">
            <h2>Friends</h2>
            <p>Connect & Compete</p>
          </div>
        </button>
      </div>

      {/* User Stats
      <div className="stats-overview">
        <div className="stat-card">
          <div className="stat-value">{mockUserStats.totalGames}</div>
          <div className="stat-label">Games Played</div>
        </div>
        <div className="stat-card">
          <div className="stat-value">{mockUserStats.gamesWon}</div>
          <div className="stat-label">Games Won</div>
        </div>
        <div className="stat-card">
          <div className="stat-value">{mockUserStats.winRate}%</div>
          <div className="stat-label">Win Rate</div>
        </div>
        <div className="stat-card">
          <div className="stat-value">{mockUserStats.currentStreak}</div>
          <div className="stat-label">Current Streak</div>
        </div>
      </div>

      <div className="recent-section">
        <h2>Recent Games</h2>
        <div className="recent-games">
          {mockRecentGames.slice(0, 5).map((game) => (
            <div key={game.id} className="game-card">
              <div className="game-info">
                <span className="game-table">{game.table}</span>
                <span className="game-date">{game.date}</span>
              </div>
              <div className={`game-result ${game.result.toLowerCase()}`}>{game.result}</div>
              <div className={`game-profit ${game.profit > 0 ? 'positive' : 'negative'}`}>
                {game.profit > 0 ? '+' : ''}
                {game.profit}
              </div>
            </div>
          ))}
        </div>
      </div> */}

      {/* Table Invite Notifications */}
      <TableInviteNotification
        invites={tableInvites}
        onAccept={handleAcceptTableInvite}
        onReject={handleRejectTableInvite}
        onDismiss={handleDismissTableInvite}
      />

      {/* Modal for additional sections */}
      {activeSection && (
        <div className="modal-overlay" onClick={closeModal}>
          <div className="modal-content" onClick={(e) => e.stopPropagation()}>
            <button className="close-btn" onClick={closeModal}>
              ✕
            </button>
            <h2>{activeSection.charAt(0).toUpperCase() + activeSection.slice(1)}</h2>
            <p>Coming soon...</p>
          </div>
        </div>
      )}
    </div>
  )
}

export default HomePage
