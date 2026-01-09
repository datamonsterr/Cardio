import React, { useState } from 'react';
import { useAuth } from '../contexts/AuthContext';
import { Link } from 'react-router-dom';
import { mockTables } from '../data/mockTables';
import type { Table } from '../types';

const TablesPage: React.FC = () => {
  const { user } = useAuth();
  const [filter, setFilter] = useState<'all' | 'available' | 'full' | 'affordable'>('all');
  const [searchTerm, setSearchTerm] = useState('');

  if (!user) return null;

  const filteredTables = mockTables.filter((table: Table) => {
    const matchesSearch = table.name.toLowerCase().includes(searchTerm.toLowerCase());
    const matchesFilter =
      filter === 'all' ||
      (filter === 'available' && table.status !== 'full') ||
      (filter === 'full' && table.status === 'full') ||
      (filter === 'affordable' && table.minBet <= user.chips);

    return matchesSearch && matchesFilter;
  });

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

  return (
    <div className="tables-container">
      <div className="tables-header">
        <h1>Poker Tables</h1>
        <p>Choose a table and start playing</p>
      </div>

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
        {filteredTables.map((table: Table) => {
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
                  <Link to="/game" className="join-btn">
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
        })}
      </div>

      {filteredTables.length === 0 && (
        <div className="no-tables">
          <p>No tables found matching your criteria</p>
        </div>
      )}
    </div>
  );
};

export default TablesPage;
