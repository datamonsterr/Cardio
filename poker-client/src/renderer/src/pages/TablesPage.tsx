import React, { useState, useEffect, useMemo } from 'react';
import { useAuth } from '../contexts/AuthContext';
import { Link, useNavigate } from 'react-router-dom';
import type { Table } from '../types';
import type { TableListResponse } from '../services/protocol';

const TablesPage: React.FC = () => {
  const { user, getTables } = useAuth();
  const navigate = useNavigate();
  const [filter, setFilter] = useState<'all' | 'available' | 'full' | 'affordable'>('all');
  const [searchTerm, setSearchTerm] = useState('');
  const [tables, setTables] = useState<Table[]>([]);
  const [loading, setLoading] = useState<boolean>(true);
  const [error, setError] = useState<string | null>(null);

  const fetchTables = async () => {
    if (!getTables) {
      setError('Tables service not available');
      setLoading(false);
      return;
    }

    try {
      setLoading(true);
      setError(null);
      const response: TableListResponse = await getTables();
      
      // Map server response to Table format
      const mappedTables: Table[] = response.tables.map((table) => {
        // Determine status based on current vs max players
        let status: 'active' | 'waiting' | 'full' = 'waiting';
        if (table.currentPlayer >= table.maxPlayer) {
          status = 'full';
        } else if (table.currentPlayer > 0) {
          status = 'active';
        }

        return {
          id: table.id.toString(),
          name: table.tableName,
          minBet: table.minBet,
          maxPlayers: table.maxPlayer,
          currentPlayers: table.currentPlayer,
          status: status,
          players: [] // Server doesn't send player details in table list
        };
      });

      setTables(mappedTables);
    } catch (err) {
      console.error('Failed to fetch tables:', err);
      setError(err instanceof Error ? err.message : 'Failed to load tables');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchTables();
  }, [getTables]);

  if (!user) return null;

  const filteredTables = useMemo(() => tables.filter((table: Table) => {
    const matchesSearch = table.name.toLowerCase().includes(searchTerm.toLowerCase());
    const matchesFilter =
      filter === 'all' ||
      (filter === 'available' && table.status !== 'full') ||
      (filter === 'full' && table.status === 'full') ||
      (filter === 'affordable' && table.minBet <= user.chips);

    return matchesSearch && matchesFilter;
  }), [tables, searchTerm, filter, user?.chips]);

  const getStatusBadge = (
    status: Table['status']
  ): { text: string; className: string } => {
    const badges = {
      active: { text: 'Active', className: 'status-active' },
      waiting: { text: 'Waiting', className: 'status-waiting' },
      full: { text: 'Full', className: 'status-full' },
    };
    return badges[status] || badges.active;
  };

  const canJoinTable = (table: Table): boolean => {
    return table.status !== 'full' && user.chips >= table.minBet;
  };

  const handleReturn = (): void => {
    navigate('/home');
  };

  return (
    <div className="tables-container">
      <div className="tables-header">
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', width: '100%' }}>
          <div>
            <h1>Poker Tables</h1>
            <p>Choose a table and start playing</p>
          </div>
          <div style={{ display: 'flex', gap: '1rem', alignItems: 'center' }}>
            <button
              onClick={handleReturn}
              className="return-button"
              title="Return to Home"
            >
              ← Return
            </button>
            <button
              onClick={fetchTables}
              disabled={loading}
              style={{
                padding: '0.75rem 1.5rem',
                background: loading ? '#2a3447' : 'linear-gradient(135deg, #ffd700 0%, #ffed4e 100%)',
                color: loading ? '#b8c5d6' : '#000',
                border: 'none',
                borderRadius: '8px',
                fontWeight: 600,
                cursor: loading ? 'not-allowed' : 'pointer',
                transition: 'all 0.3s',
              }}
              title="Refresh tables"
            >
              {loading ? 'Loading...' : '🔄 Refresh'}
            </button>
          </div>
        </div>
      </div>

      {error && (
        <div className="no-tables">
          <p>Failed to load tables: {error}</p>
        </div>
      )}

      <div className="tables-controls">
        <div className="search-box">
          <input
            type="text"
            placeholder="Search tables..."
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            className="search-input"
          />
          <span className="search-icon">🔍</span>
        </div>

        <div className="filter-buttons">
          <button
            className={`filter-btn ${filter === 'all' ? 'active' : ''}`}
            onClick={() => setFilter('all')}
          >
            All Tables
          </button>
          <button
            className={`filter-btn ${filter === 'available' ? 'active' : ''}`}
            onClick={() => setFilter('available')}
          >
            Available
          </button>
          <button
            className={`filter-btn ${filter === 'affordable' ? 'active' : ''}`}
            onClick={() => setFilter('affordable')}
          >
            Affordable
          </button>
          <button
            className={`filter-btn ${filter === 'full' ? 'active' : ''}`}
            onClick={() => setFilter('full')}
          >
            Full
          </button>
        </div>
      </div>

      <div className="tables-grid">
        {loading ? (
          <div className="no-tables">
            <p>Loading tables...</p>
          </div>
        ) : (
        filteredTables.map((table: Table) => {
          const status = getStatusBadge(table.status);
          const joinable = canJoinTable(table);

          return (
            <div key={table.id} className="table-card">
              <div className="table-card-header">
                <div>
                  <h3>{table.name}</h3>
                  <span className={`status-badge ${status.className}`}>
                    {status.text}
                  </span>
                </div>
              </div>

              <div className="table-info">
                <div className="info-row">
                  <span className="label">Min Bet:</span>
                  <span className="value">${table.minBet}</span>
                </div>
                <div className="info-row">
                  <span className="label">Players:</span>
                  <span className="value">
                    {table.currentPlayers}/{table.maxPlayers}
                  </span>
                </div>
              </div>

              <div className="table-actions">
                {joinable ? (
                  <Link 
                    to={`/game?tableId=${table.id}`} 
                    className="join-btn"
                    state={{ tableId: table.id }}
                  >
                    Join Table
                  </Link>
                ) : (
                  <button className="join-btn disabled" disabled>
                    {table.status === 'full' ? 'Table Full' : 'Insufficient Chips'}
                  </button>
                )}
              </div>
            </div>
          );
        }))}
      </div>

      {!loading && !error && filteredTables.length === 0 && (
        <div className="no-tables">
          <p>No tables found matching your criteria</p>
        </div>
      )}
    </div>
  );
};

export default TablesPage;
