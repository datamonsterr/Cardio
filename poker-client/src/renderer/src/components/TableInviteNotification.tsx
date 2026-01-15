/**
 * Table Invite Notification Component
 * Shows a toast notification when a friend invites you to join their poker table
 */

import React, { useEffect, useState } from 'react'
import '../assets/TableInviteNotification.css'

export interface TableInvite {
  id: string
  fromUsername: string
  tableId: number
  tableName?: string
  timestamp: number
}

interface TableInviteNotificationProps {
  invites: TableInvite[]
  onAccept: (invite: TableInvite) => void
  onReject: (invite: TableInvite) => void
  onDismiss: (inviteId: string) => void
}

const TableInviteNotification: React.FC<TableInviteNotificationProps> = ({
  invites,
  onAccept,
  onReject,
  onDismiss
}) => {
  const [visibleInvites, setVisibleInvites] = useState<Set<string>>(new Set())

  useEffect(() => {
    // Show new invites
    invites.forEach((invite) => {
      if (!visibleInvites.has(invite.id)) {
        setVisibleInvites((prev) => new Set(prev).add(invite.id))
      }
    })
  }, [invites])

  const handleDismiss = (inviteId: string) => {
    setVisibleInvites((prev) => {
      const newSet = new Set(prev)
      newSet.delete(inviteId)
      return newSet
    })
    setTimeout(() => onDismiss(inviteId), 300) // Wait for animation
  }

  const handleAccept = (invite: TableInvite) => {
    handleDismiss(invite.id)
    onAccept(invite)
  }

  const handleReject = (invite: TableInvite) => {
    handleDismiss(invite.id)
    onReject(invite)
  }

  const getTimeAgo = (timestamp: number): string => {
    const seconds = Math.floor((Date.now() - timestamp) / 1000)
    if (seconds < 60) return 'just now'
    const minutes = Math.floor(seconds / 60)
    if (minutes < 60) return `${minutes}m ago`
    const hours = Math.floor(minutes / 60)
    return `${hours}h ago`
  }

  return (
    <div className="table-invite-notifications">
      {invites
        .filter((invite) => visibleInvites.has(invite.id))
        .map((invite) => (
          <div
            key={invite.id}
            className="table-invite-notification slide-in"
          >
            <div className="notification-header">
              <span className="notification-icon">🎴</span>
              <span className="notification-title">Table Invite</span>
              <button
                className="notification-close"
                onClick={() => handleDismiss(invite.id)}
                aria-label="Dismiss"
              >
                ×
              </button>
            </div>
            
            <div className="notification-body">
              <p className="notification-message">
                <strong>{invite.fromUsername}</strong> invited you to join their table
                {invite.tableName && (
                  <>
                    {' '}
                    <span className="table-name">"{invite.tableName}"</span>
                  </>
                )}
              </p>
              <span className="notification-time">{getTimeAgo(invite.timestamp)}</span>
            </div>

            <div className="notification-actions">
              <button
                className="btn-reject"
                onClick={() => handleReject(invite)}
              >
                Decline
              </button>
              <button
                className="btn-accept"
                onClick={() => handleAccept(invite)}
              >
                Join Table
              </button>
            </div>
          </div>
        ))}
    </div>
  )
}

export default TableInviteNotification
