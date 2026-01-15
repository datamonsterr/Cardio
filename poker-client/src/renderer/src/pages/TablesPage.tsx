import React, { useState, useEffect, useMemo } from 'react'
import { useAuth } from '../contexts/AuthContext'
import { Link, useNavigate } from 'react-router-dom'
import type { Table, CreateTableRequestType } from '../types'
import type { TableListResponse } from '../services/protocol'

const TablesPage: React.FC = () => {
  const { user, getTables, createTable } = useAuth()
  const navigate = useNavigate()
  const [filter, setFilter] = useState<'all' | 'available' | 'full' | 'affordable'>('all')
  const [searchTerm, setSearchTerm] = useState('')
  const [tables, setTables] = useState<Table[]>([])
  const [loading, setLoading] = useState<boolean>(true)
  const [error, setError] = useState<string | null>(null)
  const [showCreateModal, setShowCreateModal] = useState<boolean>(false)
  const [creating, setCreating] = useState<boolean>(false)
  const [createForm, setCreateForm] = useState<CreateTableRequestType>({
    name: '',
    max_player: 6,
    min_bet: 10
  })

  const fetchTables = async () => {
    if (!getTables) {
      setError('Tables service not available')
      setLoading(false)
      return
    }

    try {
      setLoading(true)
      setError(null)
      const response: TableListResponse = await getTables()

      // Map server response to Table format
      const mappedTables: Table[] = response.tables.map((table) => {
        // Determine status based on current vs max players
        let status: 'active' | 'waiting' | 'full' = 'waiting'
        if (table.currentPlayer >= table.maxPlayer) {
          status = 'full'
        } else if (table.currentPlayer > 0) {
          status = 'active'
        }

        return {
          id: table.id.toString(),
          name: table.tableName,
          minBet: table.minBet,
          maxPlayers: table.maxPlayer,
          currentPlayers: table.currentPlayer,
          status: status,
          players: [] // Server doesn't send player details in table list
        }
      })

      setTables(mappedTables)
    } catch (err) {
      console.error('Failed to fetch tables:', err)
      setError(err instanceof Error ? err.message : 'Failed to load tables')
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => {
    fetchTables()
  }, [getTables])

  if (!user) return null

  const filteredTables = useMemo(
    () =>
      tables.filter((table: Table) => {
        const matchesSearch = table.name.toLowerCase().includes(searchTerm.toLowerCase())
        const matchesFilter =
          filter === 'all' ||
          (filter === 'available' && table.status !== 'full') ||
          (filter === 'full' && table.status === 'full') ||
          (filter === 'affordable' && table.minBet <= user.chips)

        return matchesSearch && matchesFilter
      }),
    [tables, searchTerm, filter, user?.chips]
  )

  const getStatusBadge = (status: Table['status']): { text: string; className: string } => {
    const badges = {
      active: { text: 'Active', className: 'status-active' },
      waiting: { text: 'Waiting', className: 'status-waiting' },
      full: { text: 'Full', className: 'status-full' }
    }
    return badges[status] || badges.active
  }

  const canJoinTable = (table: Table): boolean => {
    return table.status !== 'full' && user.chips >= table.minBet
  }

  const handleReturn = (): void => {
    navigate('/home')
  }

  const handleCreateTable = async (): Promise<void> => {
    if (!createTable) {
      setError('Create table service not available')
      return
    }

    if (!createForm.name.trim()) {
      setError('Table name is required')
      return
    }

    if (createForm.name.length > 32) {
      setError('Table name must be 32 characters or less')
      return
    }

    if (createForm.max_player < 2 || createForm.max_player > 9) {
      setError('Max players must be between 2 and 9')
      return
    }

    if (createForm.min_bet < 1) {
      setError('Min bet must be at least 1')
      return
    }

    try {
      setCreating(true)
      setError(null)
      const tableId = await createTable(createForm)
      setShowCreateModal(false)
      setCreateForm({ name: '', max_player: 6, min_bet: 10 })
      // Redirect to the game page with the created table ID
      navigate(`/game?tableId=${tableId}`)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to create table')
    } finally {
      setCreating(false)
    }
  }

  return (
    <div className="tables-page">
      <div className="tables-container">
        <div className="tables-header">
          <h1 className="tables-title">Poker Tables</h1>
          <div className="header-actions">
            <button
              onClick={fetchTables}
              disabled={loading}
              className="refresh-button"
              title="Refresh tables"
            >
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path d="M8 3a5 5 0 1 0 4.546 2.914.5.5 0 0 1 .908-.417A6 6 0 1 1 8 2v1z" />
                <path d="M8 4.466V.534a.25.25 0 0 1 .41-.192l2.36 1.966c.12.1.12.284 0 .384L8.41 4.658A.25.25 0 0 1 8 4.466z" />
              </svg>
              <span>{loading ? 'Loading...' : 'Refresh'}</span>
            </button>
            <button
              onClick={() => setShowCreateModal(true)}
              className="create-table-button"
              title="Create New Table"
            >
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path d="M8 4a.5.5 0 0 1 .5.5v3h3a.5.5 0 0 1 0 1h-3v3a.5.5 0 0 1-1 0v-3h-3a.5.5 0 0 1 0-1h3v-3A.5.5 0 0 1 8 4z" />
              </svg>
              <span>Create Table</span>
            </button>
            <button onClick={handleReturn} className="home-button" title="Return to Home">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path d="M8.707 1.5a1 1 0 0 0-1.414 0L.646 8.146a.5.5 0 0 0 .708.708L2 8.207V13.5A1.5 1.5 0 0 0 3.5 15h9a1.5 1.5 0 0 0 1.5-1.5V8.207l.646.647a.5.5 0 0 0 .708-.708L13 5.793V2.5a.5.5 0 0 0-.5-.5h-1a.5.5 0 0 0-.5.5v1.293L8.707 1.5ZM13 7.207V13.5a.5.5 0 0 1-.5.5h-9a.5.5 0 0 1-.5-.5V7.207l5-5 5 5Z" />
              </svg>
              <span>Home</span>
            </button>
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
              const status = getStatusBadge(table.status)
              const joinable = canJoinTable(table)

              return (
                <div key={table.id} className="table-card">
                  <div className="table-card-header">
                    <div>
                      <h3>{table.name}</h3>
                      <span className={`status-badge ${status.className}`}>{status.text}</span>
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
              )
            })
          )}
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
                    onChange={(e) =>
                      setCreateForm({ ...createForm, max_player: parseInt(e.target.value) || 6 })
                    }
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
                    onChange={(e) =>
                      setCreateForm({ ...createForm, min_bet: parseInt(e.target.value) || 10 })
                    }
                    className="form-input"
                  />
                </div>
                {error && <div className="form-error">{error}</div>}
                <div className="form-actions">
                  <button
                    onClick={() => setShowCreateModal(false)}
                    className="cancel-btn"
                    disabled={creating}
                  >
                    Cancel
                  </button>
                  <button onClick={handleCreateTable} className="create-btn" disabled={creating}>
                    {creating ? 'Creating...' : 'Create Table'}
                  </button>
                </div>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  )
}

export default TablesPage
