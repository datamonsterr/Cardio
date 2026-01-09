/**
 * Game service for handling game-related packets
 * Handles join table, action requests, and game state updates
 */

import { TcpClient } from '../network/TcpClient';
import {
  Packet,
  GameState,
  ActionRequest,
  ActionResult,
  JoinTableRequest,
  encodePacket,
  encodeJoinTableRequest,
  encodeActionRequest,
  decodeGameState,
  decodeActionResult,
  decodeGenericResponse,
  PACKET_TYPE,
  PROTOCOL_V1
} from '../protocol';

export interface GameConfig {
  host: string;
  port: number;
  onPacket?: (packet: Packet) => void;
  onError?: (error: Error) => void;
  onStateChange?: (state: 'disconnected' | 'connecting' | 'connected') => void;
}

export class GameService {
  private client: TcpClient | null = null;
  private config: GameConfig;
  private pendingRequests: Map<
    number,
    {
      resolve: (value: any) => void;
      reject: (error: Error) => void;
    }
  > = new Map();
  private clientSeq: number = 0;

  constructor(config: GameConfig) {
    this.config = config;
  }

  /**
   * Connect to the server
   */
  private async ensureConnected(): Promise<void> {
    if (this.client && this.client.isConnected()) {
      return;
    }

    this.client = new TcpClient({
      host: this.config.host,
      port: this.config.port,
      onPacket: (packet) => this.handlePacket(packet),
      onError: (error) => this.handleError(error),
      onStateChange: (state) => {
        if (this.config.onStateChange) {
          const mappedState = state === 'handshaking' ? 'connecting' : state;
          this.config.onStateChange(mappedState);
        }
      }
    });

    try {
      await this.client.connect();
    } catch (error) {
      throw error;
    }
  }

  /**
   * Handle incoming packets
   */
  private handlePacket(packet: Packet): void {
    const packetType = packet.header.packet_type;

    // Forward packet to user-provided handler
    if (this.config.onPacket) {
      this.config.onPacket(packet);
    }

    // Handle game-specific responses
    const pending = this.pendingRequests.get(packetType);
    if (pending) {
      this.pendingRequests.delete(packetType);

      try {
        let response: any;
        switch (packetType) {
          case PACKET_TYPE.JOIN_TABLE_RESPONSE:
            // Could be GenericResponse or GameState
            try {
              response = decodeGameState(packet.data);
            } catch {
              response = decodeGenericResponse(packet.data);
            }
            break;
          case PACKET_TYPE.ACTION_RESULT:
            response = decodeActionResult(packet.data);
            break;
          case PACKET_TYPE.UPDATE_GAMESTATE:
            response = decodeGameState(packet.data);
            break;
          default:
            response = { res: -1, msg: 'Unknown packet type' };
        }
        pending.resolve(response);
      } catch (error) {
        console.error('Failed to decode packet:', error);
        pending.reject(error instanceof Error ? error : new Error(String(error)));
      }
    }
  }

  /**
   * Handle errors
   */
  private handleError(error: Error): void {
    console.error('GameService error:', error);
    // Reject all pending requests
    this.pendingRequests.forEach((pending) => {
      pending.reject(error);
    });
    this.pendingRequests.clear();
    if (this.config.onError) {
      this.config.onError(error);
    }
  }

  /**
   * Join a table
   */
  async joinTable(request: JoinTableRequest): Promise<GameState | GenericResponse> {
    await this.ensureConnected();

    return new Promise<GameState | GenericResponse>((resolve, reject) => {
      try {
        const payload = encodeJoinTableRequest(request);
        const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.JOIN_TABLE_REQUEST, payload);

        this.pendingRequests.set(PACKET_TYPE.JOIN_TABLE_RESPONSE, {
          resolve,
          reject
        });

        this.client!.send(packet.data);

        setTimeout(() => {
          if (this.pendingRequests.has(PACKET_TYPE.JOIN_TABLE_RESPONSE)) {
            this.pendingRequests.delete(PACKET_TYPE.JOIN_TABLE_RESPONSE);
            reject(new Error('Join table request timeout'));
          }
        }, 10000);
      } catch (error) {
        reject(error instanceof Error ? error : new Error(String(error)));
      }
    });
  }

  /**
   * Send action request (fold, check, call, bet, raise)
   */
  async sendAction(request: ActionRequest): Promise<ActionResult> {
    await this.ensureConnected();

    return new Promise<ActionResult>((resolve, reject) => {
      try {
        // Increment client sequence
        request.client_seq = ++this.clientSeq;

        const payload = encodeActionRequest(request);
        const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.ACTION_REQUEST, payload);

        this.pendingRequests.set(PACKET_TYPE.ACTION_RESULT, {
          resolve,
          reject
        });

        this.client!.send(packet.data);

        setTimeout(() => {
          if (this.pendingRequests.has(PACKET_TYPE.ACTION_RESULT)) {
            this.pendingRequests.delete(PACKET_TYPE.ACTION_RESULT);
            reject(new Error('Action request timeout'));
          }
        }, 10000);
      } catch (error) {
        reject(error instanceof Error ? error : new Error(String(error)));
      }
    });
  }

  /**
   * Get the TCP client (for listening to UPDATE_GAMESTATE packets)
   */
  getClient(): TcpClient | null {
    return this.client;
  }

  /**
   * Disconnect from server
   */
  disconnect(): void {
    if (this.client) {
      this.client.disconnect();
      this.client = null;
    }
    this.pendingRequests.clear();
  }
}
