/**
 * Friend management service
 * Handles friend invites, adding friends, and friend list operations
 */

import { TcpClient } from '../network/TcpClient'
import {
  Packet,
  FriendInvite,
  Friend,
  encodePacket,
  encodeAddFriendRequest,
  encodeInviteFriendRequest,
  encodeInviteActionRequest,
  encodeGetInvitesRequest,
  decodeFriendResponse,
  decodeGetInvitesResponse,
  decodeFriendListResponse,
  PACKET_TYPE,
  RESPONSE_CODE,
  PROTOCOL_V1
} from '../protocol'

export interface FriendServiceConfig {
  client: TcpClient
}

export class FriendService {
  private client: TcpClient
  private pendingRequests: Map<
    number,
    {
      resolve: (value: any) => void
      reject: (error: Error) => void
    }
  > = new Map()

  constructor(config: FriendServiceConfig) {
    this.client = config.client

    // Intercept packets at the TcpClient level
    // Store the original onPacket handler
    const tcpClient = this.client as any
    const originalOnPacket = tcpClient.config?.onPacket

    if (tcpClient.config) {
      tcpClient.config.onPacket = (packet: Packet) => {
        console.log('[FriendService] Received packet type:', packet.header.packet_type)
        // Call our handler first (non-blocking)
        try {
          this.handlePacket(packet)
        } catch (error) {
          console.error('[FriendService] Error in handlePacket:', error)
        }
        // Then call the original handler
        if (originalOnPacket) {
          originalOnPacket(packet)
        }
      }
      console.log('[FriendService] Packet handler registered')
    } else {
      console.warn('[FriendService] Could not register packet handler - no config found')
    }
  }

  /**
   * Handle incoming friend-related packets
   */
  private handlePacket(packet: Packet): void {
    const packetType = packet.header.packet_type
    console.log(
      '[FriendService] handlePacket called for type:',
      packetType,
      'Pending:',
      Array.from(this.pendingRequests.keys())
    )
    const pending = this.pendingRequests.get(packetType)

    if (!pending) {
      return // Not a pending friend request
    }

    console.log('[FriendService] Found pending request for type:', packetType)
    this.pendingRequests.delete(packetType)

    try {
      let response: any
      switch (packetType) {
        case PACKET_TYPE.ADD_FRIEND:
          response = decodeFriendResponse(packet.data)
          console.log('Add friend response:', response)
          break
        case PACKET_TYPE.INVITE_FRIEND:
          response = decodeFriendResponse(packet.data)
          console.log('Invite friend response:', response)
          break
        case PACKET_TYPE.ACCEPT_INVITE:
          response = decodeFriendResponse(packet.data)
          console.log('Accept invite response:', response)
          break
        case PACKET_TYPE.REJECT_INVITE:
          response = decodeFriendResponse(packet.data)
          console.log('Reject invite response:', response)
          break
        case PACKET_TYPE.GET_INVITES:
          response = decodeGetInvitesResponse(packet.data)
          console.log('Get invites response:', response)
          break
        case PACKET_TYPE.GET_FRIEND_LIST:
          response = decodeFriendListResponse(packet.data)
          console.log('Friend list response:', response)
          break
        default:
          pending.reject(new Error(`Unknown packet type: ${packetType}`))
          return
      }

      pending.resolve(response)
    } catch (error) {
      console.error('Error handling friend packet:', error)
      pending.reject(error instanceof Error ? error : new Error(String(error)))
    }
  }

  /**
   * Send a request and wait for response
   */
  private async sendRequest<T>(
    packetType: number,
    payload: Uint8Array,
    timeoutMs: number = 5000
  ): Promise<T> {
    return new Promise((resolve, reject) => {
      // Set timeout
      const timeout = setTimeout(() => {
        this.pendingRequests.delete(packetType)
        reject(new Error('Request timeout'))
      }, timeoutMs)

      // Store pending request
      this.pendingRequests.set(packetType, {
        resolve: (value) => {
          clearTimeout(timeout)
          resolve(value)
        },
        reject: (error) => {
          clearTimeout(timeout)
          reject(error)
        }
      })

      console.log('[FriendService] Sending request type:', packetType)
      // Send packet
      const packet = encodePacket(PROTOCOL_V1, packetType, payload)
      this.client.send(packet.data)
    })
  }

  /**
   * Add a friend directly (creates mutual friendship)
   */
  async addFriend(username: string): Promise<void> {
    const payload = encodeAddFriendRequest(username)
    const response = await this.sendRequest<{ res: number; msg?: string }>(
      PACKET_TYPE.ADD_FRIEND,
      payload
    )

    if (response.res === RESPONSE_CODE.R_ADD_FRIEND_OK) {
      console.log(`Successfully added friend: ${username}`)
      return
    } else if (response.res === RESPONSE_CODE.R_ADD_FRIEND_ALREADY_EXISTS) {
      throw new Error('Already friends with this user')
    } else {
      throw new Error(response.msg || 'Failed to add friend')
    }
  }

  /**
   * Send a friend invite
   */
  async inviteFriend(username: string): Promise<void> {
    const payload = encodeInviteFriendRequest(username)
    const response = await this.sendRequest<{ res: number; msg?: string }>(
      PACKET_TYPE.INVITE_FRIEND,
      payload
    )

    if (response.res === RESPONSE_CODE.R_INVITE_FRIEND_OK) {
      console.log(`Successfully sent invite to: ${username}`)
      return
    } else if (response.res === RESPONSE_CODE.R_INVITE_ALREADY_SENT) {
      throw new Error('Invite already sent to this user')
    } else {
      throw new Error(response.msg || 'Failed to send invite')
    }
  }

  /**
   * Accept a friend invite
   */
  async acceptInvite(inviteId: number): Promise<void> {
    const payload = encodeInviteActionRequest(inviteId)
    const response = await this.sendRequest<{ res: number; msg?: string }>(
      PACKET_TYPE.ACCEPT_INVITE,
      payload
    )

    if (response.res === RESPONSE_CODE.R_ACCEPT_INVITE_OK) {
      console.log(`Successfully accepted invite: ${inviteId}`)
      return
    } else {
      throw new Error(response.msg || 'Failed to accept invite')
    }
  }

  /**
   * Reject a friend invite
   */
  async rejectInvite(inviteId: number): Promise<void> {
    const payload = encodeInviteActionRequest(inviteId)
    const response = await this.sendRequest<{ res: number; msg?: string }>(
      PACKET_TYPE.REJECT_INVITE,
      payload
    )

    if (response.res === RESPONSE_CODE.R_REJECT_INVITE_OK) {
      console.log(`Successfully rejected invite: ${inviteId}`)
      return
    } else {
      throw new Error(response.msg || 'Failed to reject invite')
    }
  }

  /**
   * Get pending friend invites
   */
  async getPendingInvites(): Promise<FriendInvite[]> {
    const payload = encodeGetInvitesRequest()
    const invites = await this.sendRequest<
      Array<{
        invite_id: number
        from_user_id: number
        from_username: string
        status: string
        created_at: string
      }>
    >(PACKET_TYPE.GET_INVITES, payload)

    return invites.map((invite) => ({
      invite_id: invite.invite_id,
      from_user_id: invite.from_user_id,
      from_username: invite.from_username,
      status: invite.status as 'pending' | 'accepted' | 'rejected',
      created_at: invite.created_at
    }))
  }

  /**
   * Get friend list
   */
  async getFriendList(): Promise<Friend[]> {
    const payload = new Uint8Array(0) // Empty payload
    const friends = await this.sendRequest<
      Array<{
        user_id: number
        username: string
      }>
    >(PACKET_TYPE.GET_FRIEND_LIST, payload)

    return friends.map((friend) => ({
      user_id: friend.user_id,
      username: friend.username
    }))
  }
}
