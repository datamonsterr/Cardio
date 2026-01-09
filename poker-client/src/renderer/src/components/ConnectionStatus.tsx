import React, { useState } from 'react';

export type ConnectionStatusType = 'connected' | 'disconnected' | 'connecting';

interface ConnectionStatusProps {
  status: ConnectionStatusType;
  message?: string;
}

const ConnectionStatus: React.FC<ConnectionStatusProps> = ({ status, message }) => {
  const [showTooltip, setShowTooltip] = useState(false);

  const getStatusColor = (): string => {
    switch (status) {
      case 'connected':
        return '#4CAF50'; // Green
      case 'connecting':
        return '#FFC107'; // Yellow
      case 'disconnected':
        return '#F44336'; // Red
      default:
        return '#9E9E9E'; // Gray
    }
  };

  const getStatusText = (): string => {
    switch (status) {
      case 'connected':
        return 'Connected';
      case 'connecting':
        return 'Connecting...';
      case 'disconnected':
        return 'Disconnected';
      default:
        return 'Unknown';
    }
  };

  return (
    <div
      className="connection-status"
      onMouseEnter={() => setShowTooltip(true)}
      onMouseLeave={() => setShowTooltip(false)}
      style={{
        position: 'fixed',
        top: '15px',
        left: '15px',
        zIndex: 9999,
        display: 'flex',
        alignItems: 'center',
        gap: '8px',
        cursor: 'pointer',
        pointerEvents: 'auto'
      }}
    >
      <div
        className="status-dot"
        style={{
          width: '12px',
          height: '12px',
          borderRadius: '50%',
          backgroundColor: getStatusColor(),
          boxShadow: `0 0 8px ${getStatusColor()}`,
          border: '2px solid rgba(255, 255, 255, 0.3)',
          animation: status === 'connecting' ? 'pulse 1.5s infinite' : 'none'
        }}
      />
      
      {showTooltip && (
        <div
          className="status-tooltip"
          style={{
            position: 'absolute',
            left: '25px',
            top: '-5px',
            backgroundColor: 'rgba(0, 0, 0, 0.9)',
            color: 'white',
            padding: '6px 12px',
            borderRadius: '4px',
            fontSize: '12px',
            whiteSpace: 'nowrap',
            boxShadow: '0 2px 8px rgba(0, 0, 0, 0.3)',
            pointerEvents: 'none'
          }}
        >
          {message || getStatusText()}
        </div>
      )}
    </div>
  );
};

export default ConnectionStatus;
