import React, { useState } from 'react';
import { useAuth } from '../contexts/AuthContext';
import { useNavigate } from 'react-router-dom';
import { mockUserStats, mockRecentGames } from '../data/mockTables';
import { TablesIcon, ScoreboardIcon, FriendsIcon } from '../components/icons/HomeIcons';

const HomePage: React.FC = () => {
  const { user, logout } = useAuth();
  const navigate = useNavigate();
  const [activeSection, setActiveSection] = useState<string | null>(null);

  if (!user) return null;

  const closeModal = (): void => setActiveSection(null);

  const handlePlayNow = (): void => {
    navigate('/game');
  };

  const handleViewTables = (): void => {
    navigate('/tables');
  };

  const handleLogout = (): void => {
    logout();
    navigate('/login');
  };

  const handleReturn = (): void => {
    navigate(-1); // Go back in browser history
  };

  return (
    <div className="home-container">
      <div className="home-header">
        <div className="header-content">
          <div className="header-text">
            <h1>Welcome back, {user.username}! 👋</h1>
            <p className="balance-info">Balance: ${user.chips.toLocaleString()}</p>
          </div>
          <div className="header-actions">
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
          <div className="button-icon">⚡</div>
          <div className="button-text">
            <h2>Play Now</h2>
            <p>Start a Quick Game</p>
          </div>
        </button>

        <button
          className="main-button"
          onClick={() => setActiveSection('scoreboard')}
        >
          <div className="button-icon">
            <ScoreboardIcon size={64} />
          </div>
          <div className="button-text">
            <h2>Scoreboard</h2>
            <p>View Rankings</p>
          </div>
        </button>

        <button
          className="main-button"
          onClick={() => setActiveSection('friends')}
        >
          <div className="button-icon">
            <FriendsIcon size={64} />
          </div>
          <div className="button-text">
            <h2>Friends</h2>
            <p>Connect & Compete</p>
          </div>
        </button>
      </div>

      {/* User Stats */}
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

      {/* Recent Games */}
      <div className="recent-section">
        <h2>Recent Games</h2>
        <div className="recent-games">
          {mockRecentGames.slice(0, 5).map((game) => (
            <div key={game.id} className="game-card">
              <div className="game-info">
                <span className="game-table">{game.table}</span>
                <span className="game-date">{game.date}</span>
              </div>
              <div className={`game-result ${game.result.toLowerCase()}`}>
                {game.result}
              </div>
              <div className={`game-profit ${game.profit > 0 ? 'positive' : 'negative'}`}>
                {game.profit > 0 ? '+' : ''}{game.profit}
              </div>
            </div>
          ))}
        </div>
      </div>

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
  );
};

export default HomePage;
