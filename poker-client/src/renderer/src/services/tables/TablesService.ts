/**
 * Tables service
 * Fetches live poker tables from the C server (PACKET_TABLES = 500)
 */

import { TcpClient } from '../network/TcpClient';
import {
  Packet,
  PACKET_TYPE,
  PROTOCOL_V1,
  encodePacket,
  encodeGetTablesRequest,
  decodeFullTablesResponse,
  FullTablesResponse
} from '../protocol';
import { getServerConfig } from '../../config/server';

export interface TablesConfig {
  host: string;
  port: number;
}

export class TablesService {
  private client: TcpClient | null = null;
  private config: TablesConfig;
  private pending: {
    resolve: (value: FullTablesResponse) => void;
    reject: (error: Error) => void;
  } | null = null;

  constructor(config: TablesConfig = getServerConfig()) {
    this.config = config;
  }

  private async ensureConnected(): Promise<void> {
    if (this.client && this.client.isConnected()) return;

    this.client = new TcpClient({
      host: this.config.host,
      port: this.config.port,
      onPacket: (packet) => this.handlePacket(packet),
      onError: (error) => this.handleError(error)
    });

    await this.client.connect();
  }

  private handlePacket(packet: Packet): void {
    if (packet.header.packet_type !== PACKET_TYPE.PACKET_TABLES) return;
    if (!this.pending) return;

    const pending = this.pending;
    this.pending = null;

    try {
      const decoded = decodeFullTablesResponse(packet.data);
      pending.resolve(decoded);
    } catch (error) {
      pending.reject(error instanceof Error ? error : new Error(String(error)));
    }
  }

  private handleError(error: Error): void {
    if (this.pending) {
      const pending = this.pending;
      this.pending = null;
      pending.reject(error);
    }
  }

  async getTables(): Promise<FullTablesResponse> {
    await this.ensureConnected();

    if (!this.client) {
      throw new Error('Client is not initialized');
    }

    if (this.pending) {
      throw new Error('A tables request is already in flight');
    }

    return new Promise<FullTablesResponse>((resolve, reject) => {
      try {
        this.pending = { resolve, reject };

        const payload = encodeGetTablesRequest();
        const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.PACKET_TABLES, payload);
        this.client!.send(packet.data);

        setTimeout(() => {
          if (this.pending) {
            this.pending = null;
            reject(new Error('Get tables request timeout'));
          }
        }, 8000);
      } catch (error) {
        this.pending = null;
        reject(error instanceof Error ? error : new Error(String(error)));
      }
    });
  }

  disconnect(): void {
    if (this.client) {
      this.client.disconnect();
      this.client = null;
    }
    this.pending = null;
  }
}
