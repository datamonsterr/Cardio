/**
 * Table Invite Service
 * Handles inviting friends to join poker tables
 */

import { TcpClient } from '../network/TcpClient'
import {
  Packet,
  encodePacket,
  encodeTableInviteRequest,
  decodeTableInviteResponse,
  TableInviteResponse,
  PACKET_INVITE_TO_TABLE,
  RESPONSE_CODE,
  PROTOCOL_V1
} from '../protocol'

export interface TableInviteServiceConfig {
  client: TcpClient
}

export class TableInviteService {
  private client: TcpClient
  private pendingRequests: Map<
    number,
    {
      resolve: (value: TableInviteResponse) => void
      reject: (error: Error) => void
    }
  > = new Map()

  constructor(config: TableInviteServiceConfig) {
    this.client = config.client

    // Intercept packets at the TcpClient level
    const tcpClient = this.client as any
    const originalOnPacket = tcpClient.config?.onPacket

    if (tcpClient.config) {
      tcpClient.config.onPacket = (packet: Packet) => {
        // Call our handler first (non-blocking)
        try {
          this.handlePacket(packet)
        } catch (error) {
          console.error('[TableInviteService] Error in handlePacket:', error)
        }
        // Then call the original handler
        if (originalOnPacket) {
          originalOnPacket(packet)
        }
      }
    }
  }

  /**
   * Handle incoming table invite response packets
   */
  private handlePacket(packet: Packet): void {
    const packetType = packet.header.packet_type
    console.log('[TableInviteService] Received packet type:', packetType)
    
    const pending = this.pendingRequests.get(packetType)

    if (!pending) {
      console.log('[TableInviteService] No pending request for packet type:', packetType)
      return // Not a pending table invite request
    }

    this.pendingRequests.delete(packetType)

    try {
      const response = decodeTableInviteResponse(packet.data)
      console.log('[TableInviteService] Decoded response:', response)
      pending.resolve(response)
    } catch (error) {
      console.error('[TableInviteService] Failed to decode response:', error)
      pending.reject(
        error instanceof Error ? error : new Error('Failed to decode table invite response')
      )
    }
  }

  /**
   * Send a table invite to a friend
   * @param friendUsername Username of the friend to invite
   * @param tableId ID of the table to invite them to
   * @returns Promise that resolves with the server response
   */
  async inviteToTable(friendUsername: string, tableId: number): Promise<TableInviteResponse> {
    console.log(`[TableInviteService] Inviting ${friendUsername} to table ${tableId}`)

    const payload = encodeTableInviteRequest(friendUsername, tableId)
    const packet = encodePacket(PROTOCOL_V1, PACKET_INVITE_TO_TABLE, payload)

    return new Promise((resolve, reject) => {
      // Store the promise handlers
      this.pendingRequests.set(PACKET_INVITE_TO_TABLE, { resolve, reject })

      // Set timeout for the request
      const timeout = setTimeout(() => {
        this.pendingRequests.delete(PACKET_INVITE_TO_TABLE)
        reject(new Error('Table invite request timed out'))
      }, 5000) // 5 second timeout

      // Send the packet
      try {
        this.client.send(packet.data)
        console.log('[TableInviteService] Table invite request sent')
      } catch (error) {
        clearTimeout(timeout)
        this.pendingRequests.delete(PACKET_INVITE_TO_TABLE)
        reject(error)
        return
      }

      // Clear timeout when request completes
      const originalResolve = resolve
      const originalReject = reject

      const wrappedResolve = (value: TableInviteResponse) => {
        clearTimeout(timeout)
        originalResolve(value)
      }

      const wrappedReject = (error: Error) => {
        clearTimeout(timeout)
        originalReject(error)
      }

      this.pendingRequests.set(PACKET_INVITE_TO_TABLE, {
        resolve: wrappedResolve,
        reject: wrappedReject
      })
    })
  }

  /**
   * Check if the invite was successful
   */
  static isSuccess(response: TableInviteResponse): boolean {
    return response.result === RESPONSE_CODE.R_INVITE_TO_TABLE_OK
  }

  /**
   * Get a user-friendly error message from the response
   */
  static getErrorMessage(response: TableInviteResponse): string {
    switch (response.result) {
      case RESPONSE_CODE.R_INVITE_TO_TABLE_OK:
        return 'Invite sent successfully'
      case RESPONSE_CODE.R_INVITE_TO_TABLE_NOT_FRIENDS:
        return 'You are not friends with this player'
      case RESPONSE_CODE.R_INVITE_TO_TABLE_ALREADY_IN_GAME:
        return 'Player is already in a game'
      case RESPONSE_CODE.R_INVITE_TO_TABLE_NOT_OK:
        return 'Failed to send invite'
      default:
        return response.message || 'Unknown error'
    }
  }
}
