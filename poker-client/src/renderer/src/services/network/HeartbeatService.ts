/**
 * Heartbeat service
 * Monitors server connection health using PING/PONG
 */

import { TcpClient } from './TcpClient';
import { encodePacket, PACKET_TYPE, PROTOCOL_V1 } from '../protocol';

export type HeartbeatStatus = 'connected' | 'reconnecting' | 'disconnected';

export interface HeartbeatConfig {
  onStatusChange?: (status: HeartbeatStatus, message?: string) => void;
}

export class HeartbeatService {
  private client: TcpClient | null = null;
  private config: HeartbeatConfig;
  private heartbeatInterval: NodeJS.Timeout | null = null;
  private pingAttempts: number = 0;
  private maxPingAttempts: number = 6; // 6 attempts * 5 seconds = 30 seconds
  private currentStatus: HeartbeatStatus = 'disconnected';
  private waitingForPong: boolean = false;

  constructor(config: HeartbeatConfig) {
    this.config = config;
  }

  /**
   * Start heartbeat monitoring
   */
  start(client: TcpClient): void {
    this.stop();
    this.client = client;
    this.pingAttempts = 0;
    this.waitingForPong = false;

    console.log('HeartbeatService: Starting heartbeat monitoring');
    
    // Send first PING immediately
    this.sendPing();

    // Send PING every 5 seconds
    this.heartbeatInterval = setInterval(() => {
      this.sendPing();
    }, 5000);
  }

  /**
   * Stop heartbeat monitoring
   */
  stop(): void {
    if (this.heartbeatInterval) {
      clearInterval(this.heartbeatInterval);
      this.heartbeatInterval = null;
    }
    this.pingAttempts = 0;
    this.waitingForPong = false;
    this.client = null;
    console.log('HeartbeatService: Stopped heartbeat monitoring');
  }

  /**
   * Send PING to server
   */
  private sendPing(): void {
    if (!this.client || !this.client.isConnected()) {
      console.log('HeartbeatService: Client not connected');
      this.setStatus('disconnected', 'Not connected to server');
      this.stop();
      return;
    }

    // If we have pending PINGs without PONG, we're in reconnecting state
    if (this.pingAttempts > 0 && this.waitingForPong) {
      console.warn(`HeartbeatService: Previous PING not answered, incrementing attempt`);
      this.pingAttempts++;
      
      // Check if we exceeded max attempts
      if (this.pingAttempts > this.maxPingAttempts) {
        console.warn(`HeartbeatService: No PONG after ${this.maxPingAttempts} attempts (30 seconds)`);
        this.setStatus('disconnected', 'Server not responding');
        this.stop();
        return;
      }
      
      // Update status to reconnecting
      this.setStatus('reconnecting', `Checking connection... (${this.pingAttempts}/${this.maxPingAttempts})`);
    } else {
      // First PING or after successful PONG
      this.pingAttempts = 1;
      this.waitingForPong = true;
      console.log('HeartbeatService: Sending PING');
    }

    try {
      // Encode and send PING packet
      const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.PING, null);
      this.client.send(packet.data);
    } catch (error) {
      console.error('HeartbeatService: Failed to send PING:', error);
      this.setStatus('disconnected', 'Failed to send PING');
    }
  }

  /**
   * Handle PONG received from server
   */
  onPongReceived(): void {
    if (!this.waitingForPong) {
      return;
    }

    console.log('HeartbeatService: Received PONG - connection is healthy');
    
    // Reset attempts counter
    this.pingAttempts = 0;
    this.waitingForPong = false;

    // Update status to connected
    this.setStatus('connected', 'Connected to server');
  }

  /**
   * Set status and notify listeners
   */
  private setStatus(status: HeartbeatStatus, message?: string): void {
    if (this.currentStatus !== status) {
      this.currentStatus = status;
      console.log(`HeartbeatService: Status changed to ${status}${message ? ': ' + message : ''}`);
      if (this.config.onStatusChange) {
        this.config.onStatusChange(status, message);
      }
    }
  }

  /**
   * Get current status
   */
  getStatus(): HeartbeatStatus {
    return this.currentStatus;
  }
}
