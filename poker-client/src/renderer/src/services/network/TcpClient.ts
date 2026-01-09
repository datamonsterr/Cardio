/**
 * TCP Client for raw socket communication with game server
 * Uses Node.js net module through Electron's main process
 */

import {
  Packet,
  decodePacket,
  decodeHeader,
  HEADER_SIZE,
  createHandshakeRequest,
  parseHandshakeResponse,
  PROTOCOL_V1
} from '../protocol';

export type ConnectionState = 'disconnected' | 'connecting' | 'handshaking' | 'connected';

export interface TcpClientConfig {
  host: string;
  port: number;
  timeout?: number;
  onPacket?: (packet: Packet) => void;
  onError?: (error: Error) => void;
  onStateChange?: (state: ConnectionState) => void;
}

export class TcpClient {
  private socket: any = null;
  private config: TcpClientConfig;
  private state: ConnectionState = 'disconnected';
  private buffer: Uint8Array = new Uint8Array(0);
  private expectedLength: number | null = null;
  private handshakePromise: Promise<void> | null = null;
  private handshakeResolve: (() => void) | null = null;
  private handshakeReject: ((error: Error) => void) | null = null;

  constructor(config: TcpClientConfig) {
    this.config = config;
  }

  /**
   * Connect to the server and perform handshake
   */
  async connect(): Promise<void> {
    if (this.state !== 'disconnected') {
      throw new Error(`Cannot connect: already ${this.state}`);
    }

    return new Promise((resolve, reject) => {
      this.setState('connecting');

      // Use Electron's IPC to create socket in main process
      // For now, we'll use the renderer process net module (with nodeIntegration)
      const net = window.require('net');

      this.socket = new net.Socket();
      this.socket.setTimeout(this.config.timeout || 30000);

      // Set up event handlers
      this.socket.on('connect', async () => {
        try {
          await this.performHandshake();
          this.setState('connected');
          resolve();
        } catch (error) {
          reject(error instanceof Error ? error : new Error(String(error)));
        }
      });

      this.socket.on('data', (data: Buffer) => {
        this.handleData(new Uint8Array(data));
      });

      this.socket.on('error', (error: Error) => {
        this.handleError(error);
        reject(error);
      });

      this.socket.on('close', () => {
        this.handleClose();
      });

      this.socket.on('timeout', () => {
        const error = new Error('Connection timeout');
        this.handleError(error);
        reject(error);
      });

      // Connect to server
      this.socket.connect(this.config.port, this.config.host);
    });
  }

  /**
   * Perform protocol handshake
   */
  private async performHandshake(): Promise<void> {
    this.setState('handshaking');

    return new Promise((resolve, reject) => {
      this.handshakeResolve = resolve;
      this.handshakeReject = reject;

      this.handshakePromise = new Promise<void>((res, rej) => {
        this.handshakeResolve = res;
        this.handshakeReject = rej;
      });

      // Send handshake request
      const handshakeRequest = createHandshakeRequest(PROTOCOL_V1);
      this.socket.write(Buffer.from(handshakeRequest));

      // Set timeout for handshake
      const timeoutId = setTimeout(() => {
        if (this.handshakeReject) {
          this.handshakeReject(new Error('Handshake timeout'));
          this.handshakeResolve = null;
          this.handshakeReject = null;
        }
      }, 5000);

      this.handshakePromise
        .then(() => {
          clearTimeout(timeoutId);
          resolve();
        })
        .catch((error) => {
          clearTimeout(timeoutId);
          reject(error);
        });
    });
  }

  /**
   * Handle incoming data
   */
  private handleData(data: Uint8Array): void {
    // Handle handshake response separately
    if (this.state === 'handshaking' && this.handshakeResolve && this.handshakeReject) {
      try {
        const response = parseHandshakeResponse(data);
        if (response.code === 0) {
          this.handshakeResolve();
        } else {
          this.handshakeReject(new Error(`Handshake failed with code ${response.code}`));
        }
      } catch (error) {
        this.handshakeReject(error instanceof Error ? error : new Error(String(error)));
      }
      this.handshakeResolve = null;
      this.handshakeReject = null;
      return;
    }

    // Append new data to buffer
    const newBuffer = new Uint8Array(this.buffer.length + data.length);
    newBuffer.set(this.buffer);
    newBuffer.set(data, this.buffer.length);
    this.buffer = newBuffer;

    // Process complete packets
    this.processBuffer();
  }

  /**
   * Process buffered data and extract complete packets
   */
  private processBuffer(): void {
    while (this.buffer.length > 0) {
      // If we don't know the expected length yet, try to read header
      if (this.expectedLength === null) {
        if (this.buffer.length < HEADER_SIZE) {
          // Not enough data for header yet
          return;
        }

        try {
          const header = decodeHeader(this.buffer);
          this.expectedLength = header.packet_len;
        } catch (error) {
          this.handleError(
            error instanceof Error ? error : new Error('Failed to decode header')
          );
          this.buffer = new Uint8Array(0);
          return;
        }
      }

      // Check if we have a complete packet
      if (this.buffer.length < this.expectedLength) {
        // Not enough data yet
        return;
      }

      // Extract complete packet
      const packetData = this.buffer.slice(0, this.expectedLength);
      this.buffer = this.buffer.slice(this.expectedLength);
      this.expectedLength = null;

      // Decode and handle packet
      try {
        const packet = decodePacket(packetData);
        this.handlePacket(packet);
      } catch (error) {
        this.handleError(
          error instanceof Error ? error : new Error('Failed to decode packet')
        );
      }
    }
  }

  /**
   * Handle a complete packet
   */
  private handlePacket(packet: Packet): void {
    if (this.config.onPacket) {
      this.config.onPacket(packet);
    }
  }

  /**
   * Handle errors
   */
  private handleError(error: Error): void {
    if (this.config.onError) {
      this.config.onError(error);
    }
  }

  /**
   * Handle connection close
   */
  private handleClose(): void {
    this.setState('disconnected');
    this.buffer = new Uint8Array(0);
    this.expectedLength = null;
    this.socket = null;
  }

  /**
   * Send raw bytes to server
   */
  send(data: Uint8Array): void {
    if (this.state !== 'connected') {
      throw new Error(`Cannot send: connection is ${this.state}`);
    }

    if (!this.socket) {
      throw new Error('Socket is not initialized');
    }

    this.socket.write(Buffer.from(data));
  }

  /**
   * Disconnect from server
   */
  disconnect(): void {
    if (this.socket) {
      this.socket.destroy();
      this.socket = null;
    }
    this.setState('disconnected');
  }

  /**
   * Get current connection state
   */
  getState(): ConnectionState {
    return this.state;
  }

  /**
   * Check if connected
   */
  isConnected(): boolean {
    return this.state === 'connected';
  }

  /**
   * Update connection state
   */
  private setState(state: ConnectionState): void {
    if (this.state !== state) {
      this.state = state;
      if (this.config.onStateChange) {
        this.config.onStateChange(state);
      }
    }
  }
}
