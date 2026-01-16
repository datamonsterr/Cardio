/**
 * Leave Table Service
 * Sends PACKET_LEAVE_TABLE request to server (packet type 700)
 */

import { TcpClient } from '../network/TcpClient'
import {
  Packet,
  PROTOCOL_V1,
  encodePacket,
  encodeLeaveTableRequest,
  PACKET_LEAVE_TABLE
} from '../protocol'
import { getServerConfig } from '../../config/server'
import { decode as msgpackDecode } from '@msgpack/msgpack'

export interface LeaveTableConfig {
  host: string
  port: number
}

export interface LeaveTableResponse {
  success: boolean
  message?: string
}

export class LeaveTableService {
  private client: TcpClient | null = null
  private config: LeaveTableConfig
  private pending: {
    resolve: (value: LeaveTableResponse) => void
    reject: (error: Error) => void
  } | null = null

  constructor(config: LeaveTableConfig = getServerConfig()) {
    this.config = config
  }

  private async ensureConnected(): Promise<void> {
    if (this.client && this.client.isConnected()) return

    this.client = new TcpClient({
      host: this.config.host,
      port: this.config.port,
      onPacket: (packet) => this.handlePacket(packet),
      onError: (error) => this.handleError(error)
    })

    await this.client.connect()
  }

  private handlePacket(packet: Packet): void {
    if (packet.header.packet_type !== PACKET_LEAVE_TABLE) return
    if (!this.pending) return

    const pending = this.pending
    this.pending = null

    try {
      // Decode msgpack response: { res: number }
      const response = msgpackDecode(packet.data) as { res: number }

      if (response.res === 0) {
        pending.resolve({ success: true })
      } else {
        pending.resolve({ success: false, message: 'Failed to leave table' })
      }
    } catch (error) {
      pending.reject(error instanceof Error ? error : new Error(String(error)))
    }
  }

  private handleError(error: Error): void {
    if (this.pending) {
      const pending = this.pending
      this.pending = null
      pending.reject(error)
    }
  }

  async leaveTable(tableId: number): Promise<LeaveTableResponse> {
    await this.ensureConnected()

    if (!this.client) {
      throw new Error('Client is not initialized')
    }

    if (this.pending) {
      throw new Error('A leave table request is already in flight')
    }

    return new Promise<LeaveTableResponse>((resolve, reject) => {
      try {
        this.pending = { resolve, reject }

        const payload = encodeLeaveTableRequest(tableId)
        const packet = encodePacket(PROTOCOL_V1, PACKET_LEAVE_TABLE, payload)
        this.client!.send(packet.data)

        setTimeout(() => {
          if (this.pending) {
            this.pending = null
            reject(new Error('Leave table request timeout'))
          }
        }, 5000)
      } catch (error) {
        this.pending = null
        reject(error instanceof Error ? error : new Error(String(error)))
      }
    })
  }

  disconnect(): void {
    if (this.client) {
      this.client.disconnect()
      this.client = null
    }
    this.pending = null
  }
}
