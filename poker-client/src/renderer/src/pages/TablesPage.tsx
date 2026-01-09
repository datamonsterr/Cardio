import React, { useState, useEffect, useMemo } from 'react';
import { useAuth } from '../contexts/AuthContext';
import { useNavigate } from 'react-router-dom';
import type { Table, CreateTableRequestType } from '../types';
import type { TableListResponse } from '../services/protocol';

const TablesPage: React.FC = () => {
  const { user, getTables, createTable } = useAuth();
  const navigate = useNavigate();
  const [filter, setFilter] = useState<'all' | 'available' | 'full' | 'affordable'>('all');
  const [searchTerm, setSearchTerm] = useState('');
  const [tables, setTables] = useState<Table[]>([]);
  const [loading, setLoading] = useState<boolean>(true);
  const [error, setError] = useState<string | null>(null);
  const [showCreateModal, setShowCreateModal] = useState<boolean>(false);
  const [creating, setCreating] = useState<boolean>(false);
  const [createForm, setCreateForm] = useState<CreateTableRequestType>({
    name: '',
    max_player: 6,
    min_bet: 10
  });

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

  const handleCreateTable = async (): Promise<void> => {
    if (!createTable) {
      setError('Create table service not available');
      return;
    }

    if (!createForm.name.trim()) {
      setError('Table name is required');
      return;
    }

    if (createForm.name.length > 32) {
      setError('Table name must be 32 characters or less');
      return;
    }

    if (createForm.max_player < 2 || createForm.max_player > 9) {
      setError('Max players must be between 2 and 9');
      return;
    }

    if (createForm.min_bet < 1) {
      setError('Min bet must be at least 1');
      return;
    }

    try {
      setCreating(true);
      setError(null);
      const tableName = createForm.name;
      await createTable(createForm);
      setShowCreateModal(false);
      const savedForm = { ...createForm };
      setCreateForm({ name: '', max_player: 6, min_bet: 10 });
      
      // Refresh tables to get the new table ID
      await fetchTables();
      
      // Wait a bit for server to update table list
      await new Promise(resolve => setTimeout(resolve, 300));
      await fetchTables();
      
      // Find the newly created table (by name) and navigate to game page
      const newTable = tables.find(t => t.name === tableName);
      
      if (newTable) {
        navigate(`/game?tableId=${newTable.id}`);
      } else {
        // If we can't find the table immediately, try fetching one more time
        await new Promise(resolve => setTimeout(resolve, 500));
        await fetchTables();
        const retryTable = tables.find(t => t.name === tableName);
        if (retryTable) {
          navigate(`/game?tableId=${retryTable.id}`);
        } else {
          // Table created but couldn't find it - user can manually join
          console.warn('Table created but not found in list, user should manually join');
        }
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to create table');
    } finally {
      setCreating(false);
    }
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
              onClick={() => setShowCreateModal(true)}
              className="create-table-button"
              title="Create New Table"
            >
              + Create Table
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
                  <span className="label">Min Bet (Big Blind):</span>
                  <span className="value">${table.minBet.toLocaleString()}</span>
                </div>
                <div className="info-row">
                  <span className="label">Small Blind:</span>
                  <span className="value">${Math.floor(table.minBet / 2).toLocaleString()}</span>
                </div>
                <div className="info-row">
                  <span className="label">Players:</span>
                  <span className="value">
                    <span className="player-count">{table.currentPlayers}</span>
                    <span className="player-separator">/</span>
                    <span className="player-max">{table.maxPlayers}</span>
                  </span>
                </div>
                <div className="info-row">
                  <span className="label">Available Seats:</span>
                  <span className="value">
                    {table.maxPlayers - table.currentPlayers} seats left
                  </span>
                </div>
              </div>

              <div className="table-actions">
                {joinable ? (
                  <button
                    onClick={() => navigate(`/game?tableId=${table.id}`)}
                    className="join-btn"
                  >
                    Join Table
                  </button>
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

      {/* Create Table Modal */}
      {showCreateModal && (
        <div className="modal-overlay" onClick={() => setShowCreateModal(false)}>
          <div className="modal-content create-table-modal" onClick={(e) => e.stopPropagation()}>
            <button className="close-btn" onClick={() => setShowCreateModal(false)}>
              ✕
            </button>
            <h2>Create New Table</h2>
            <div className="create-table-form">
              <div className="form-group">
                <label htmlFor="table-name">Table Name</label>
                <input
                  id="table-name"
                  type="text"
                  value={createForm.name}
                  onChange={(e) => setCreateForm({ ...createForm, name: e.target.value })}
                  placeholder="Enter table name (max 32 chars)"
                  maxLength={32}
                  className="form-input"
                />
              </div>
              <div className="form-group">
                <label htmlFor="max-players">Max Players</label>
                <input
                  id="max-players"
                  type="number"
                  min="2"
                  max="9"
                  value={createForm.max_player}
                  onChange={(e) => setCreateForm({ ...createForm, max_player: parseInt(e.target.value) || 6 })}
                  className="form-input"
                />
              </div>
              <div className="form-group">
                <label htmlFor="min-bet">Min Bet (Big Blind)</label>
                <input
                  id="min-bet"
                  type="number"
                  min="1"
                  value={createForm.min_bet}
                  onChange={(e) => setCreateForm({ ...createForm, min_bet: parseInt(e.target.value) || 10 })}
                  className="form-input"
                />
              </div>
              {error && (
                <div className="form-error">
                  {error}
                </div>
              )}
              <div className="form-actions">
                <button
                  onClick={() => setShowCreateModal(false)}
                  className="cancel-btn"
                  disabled={creating}
                >
                  Cancel
                </button>
                <button
                  onClick={handleCreateTable}
                  className="create-btn"
                  disabled={creating}
                >
                  {creating ? 'Creating...' : 'Create Table'}
                </button>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

export default TablesPage;
