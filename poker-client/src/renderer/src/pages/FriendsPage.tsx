import React, { useState, useEffect } from 'react'
import { FriendService } from '../services/friend'
import { FriendInvite, Friend } from '../services/protocol/types'
import { useAuth } from '../contexts/AuthContext'
import { useNavigate } from 'react-router-dom'
import '../assets/friends.css'

export const FriendsPage: React.FC = () => {
  const { getClient } = useAuth()
  const navigate = useNavigate()
  const [friendService, setFriendService] = useState<FriendService | null>(null)

  const [friends, setFriends] = useState<Friend[]>([])
  const [invites, setInvites] = useState<FriendInvite[]>([])
  const [activeTab, setActiveTab] = useState<'friends' | 'invites'>('friends')

  const [usernameInput, setUsernameInput] = useState('')
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [success, setSuccess] = useState<string | null>(null)

  // Initialize friend service
  useEffect(() => {
    const client = getClient?.()
    if (client) {
      const service = new FriendService({ client })
      setFriendService(service)
      loadData(service)
    }
  }, [getClient])

  const loadData = async (service: FriendService) => {
    try {
      setLoading(true)
      const [friendsData, invitesData] = await Promise.all([
        service.getFriendList(),
        service.getPendingInvites()
      ])
      setFriends(friendsData)
      setInvites(invitesData)
    } catch (err) {
      console.error('Failed to load friend data:', err)
      setError(err instanceof Error ? err.message : 'Failed to load data')
    } finally {
      setLoading(false)
    }
  }

  const handleInviteFriend = async () => {
    if (!friendService || !usernameInput.trim()) return

    try {
      setLoading(true)
      setError(null)
      setSuccess(null)

      await friendService.inviteFriend(usernameInput.trim())
      setSuccess(`Invite sent to ${usernameInput}!`)
      setUsernameInput('')
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to send invite')
    } finally {
      setLoading(false)
    }
  }

  const handleAcceptInvite = async (inviteId: number, username: string) => {
    if (!friendService) return

    try {
      setLoading(true)
      setError(null)
      setSuccess(null)

      await friendService.acceptInvite(inviteId)
      setSuccess(`Accepted invite from ${username}!`)

      // Reload data
      await loadData(friendService)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to accept invite')
    } finally {
      setLoading(false)
    }
  }

  const handleRejectInvite = async (inviteId: number, username: string) => {
    if (!friendService) return

    try {
      setLoading(true)
      setError(null)
      setSuccess(null)

      await friendService.rejectInvite(inviteId)
      setSuccess(`Rejected invite from ${username}`)

      // Reload data
      await loadData(friendService)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to reject invite')
    } finally {
      setLoading(false)
    }
  }

  const handleRefresh = () => {
    if (friendService) {
      loadData(friendService)
    }
  }

  const handleBackToHome = () => {
    navigate('/')
  }

  return (
    <div className="friends-page">
      <div className="friends-container">
        <div className="friends-header">
          <h1 className="friends-title">Friends</h1>
          <div className="header-actions">
            <button
              onClick={handleRefresh}
              className="refresh-button"
              disabled={loading}
              title="Refresh data"
            >
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path d="M8 3a5 5 0 1 0 4.546 2.914.5.5 0 0 1 .908-.417A6 6 0 1 1 8 2v1z" />
                <path d="M8 4.466V.534a.25.25 0 0 1 .41-.192l2.36 1.966c.12.1.12.284 0 .384L8.41 4.658A.25.25 0 0 1 8 4.466z" />
              </svg>
              <span>Refresh</span>
            </button>
            <button onClick={handleBackToHome} className="home-button" title="Back to home">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path d="M8.707 1.5a1 1 0 0 0-1.414 0L.646 8.146a.5.5 0 0 0 .708.708L2 8.207V13.5A1.5 1.5 0 0 0 3.5 15h9a1.5 1.5 0 0 0 1.5-1.5V8.207l.646.647a.5.5 0 0 0 .708-.708L13 5.793V2.5a.5.5 0 0 0-.5-.5h-1a.5.5 0 0 0-.5.5v1.293L8.707 1.5ZM13 7.207V13.5a.5.5 0 0 1-.5.5h-9a.5.5 0 0 1-.5-.5V7.207l5-5 5 5Z" />
              </svg>
              <span>Home</span>
            </button>
          </div>
        </div>

        {/* Tabs */}
        <div className="friends-tabs">
          <button
            className={`friends-tab ${activeTab === 'friends' ? 'active' : ''}`}
            onClick={() => setActiveTab('friends')}
          >
            Friends ({friends.length})
          </button>
          <button
            className={`friends-tab ${activeTab === 'invites' ? 'active' : ''}`}
            onClick={() => setActiveTab('invites')}
          >
            Invites ({invites.length})
          </button>
        </div>

        {/* Error/Success Messages */}
        {error && <div className="message-box error-message">❌ {error}</div>}
        {success && <div className="message-box success-message">✅ {success}</div>}

        {/* Invite Friend Form */}
        <div className="invite-form">
          <input
            type="text"
            placeholder="Enter username to invite..."
            value={usernameInput}
            onChange={(e) => setUsernameInput(e.target.value)}
            onKeyPress={(e) => e.key === 'Enter' && handleInviteFriend()}
            className="invite-input"
            disabled={loading}
          />
          <button
            onClick={handleInviteFriend}
            className="invite-button"
            disabled={loading || !usernameInput.trim()}
          >
            Send Invite
          </button>
        </div>

        {/* Content */}
        <div className="friends-content">
          {loading && <div className="loading-text">Loading...</div>}

          {!loading && activeTab === 'friends' && (
            <div className="friends-list">
              {friends.length === 0 ? (
                <div className="empty-state">
                  <p>No friends yet. Start by inviting someone!</p>
                </div>
              ) : (
                friends.map((friend) => (
                  <div key={friend.user_id} className="friend-card">
                    <div className="friend-info">
                      <div className="avatar">{friend.username.charAt(0).toUpperCase()}</div>
                      <div>
                        <div className="friend-name">{friend.username}</div>
                        <div className="friend-id">ID: {friend.user_id}</div>
                      </div>
                    </div>
                  </div>
                ))
              )}
            </div>
          )}

          {!loading && activeTab === 'invites' && (
            <div className="invites-list">
              {invites.length === 0 ? (
                <div className="empty-state">
                  <p>No pending invites</p>
                </div>
              ) : (
                invites.map((invite) => (
                  <div key={invite.invite_id} className="invite-card">
                    <div className="invite-info">
                      <div className="avatar">{invite.from_username.charAt(0).toUpperCase()}</div>
                      <div style={{ flex: 1 }}>
                        <div className="invite-name">{invite.from_username}</div>
                        <div className="invite-date">
                          {new Date(invite.created_at).toLocaleDateString()}
                        </div>
                      </div>
                    </div>
                    <div className="invite-actions">
                      <button
                        onClick={() => handleAcceptInvite(invite.invite_id, invite.from_username)}
                        className="accept-button"
                        disabled={loading}
                      >
                        ✓ Accept
                      </button>
                      <button
                        onClick={() => handleRejectInvite(invite.invite_id, invite.from_username)}
                        className="reject-button"
                        disabled={loading}
                      >
                        ✗ Reject
                      </button>
                    </div>
                  </div>
                ))
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  )
}
