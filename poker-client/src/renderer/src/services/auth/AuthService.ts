/**
 * Authentication service
 * Handles login, signup, and maintains user session
 */

import { TcpClient } from '../network/TcpClient';
import {
  Packet,
  LoginResponse,
  SignupResponse,
  TableListResponse,
  GenericResponse,
  CreateTableRequest,
  encodePacket,
  encodeLoginRequest,
  encodeSignupRequest,
  encodeGetTablesRequest,
  encodeCreateTableRequest,
  decodeLoginResponse,
  decodeSignupResponse,
  decodeTableListResponse,
  decodeGenericResponse,
  PACKET_TYPE,
  PROTOCOL_V1,
  SignupRequest
} from '../protocol';

export interface User {
  user_id: number;
  username: string;
  email: string;
  phone: string;
  full_name: string;
  country: string;
  gender: string;
  dob: string;
  balance: number;
  avatar_url: string;
}

export type ConnectionStatus = 'disconnected' | 'connecting' | 'handshaking' | 'connected' | 'error';

export interface AuthConfig {
  host: string;
  port: number;
  onDisconnect?: () => void;
  onPacket?: (packet: Packet) => void;
  onConnectionStatusChange?: (status: ConnectionStatus, message?: string) => void;
}

export class AuthService {
  private client: TcpClient | null = null;
  private currentUser: User | null = null;
  private config: AuthConfig;
  private connectionStatus: ConnectionStatus = 'disconnected';
  private pendingRequests: Map<
    number,
    {
      resolve: (value: any) => void;
      reject: (error: Error) => void;
    }
  > = new Map();

  constructor(config: AuthConfig) {
    this.config = config;
  }

  /**
   * Connect to the server
   */
  private async ensureConnected(): Promise<void> {
    if (this.client && this.client.isConnected()) {
      return;
    }

    this.setConnectionStatus('connecting', 'Connecting to server...');

    this.client = new TcpClient({
      host: this.config.host,
      port: this.config.port,
      onPacket: (packet) => this.handlePacket(packet),
      onError: (error) => this.handleError(error),
      onStateChange: (state) => {
        if (state === 'connecting') {
          this.setConnectionStatus('connecting', 'Connecting to server...');
        } else if (state === 'handshaking') {
          this.setConnectionStatus('handshaking', 'Performing handshake...');
        } else if (state === 'connected') {
          this.setConnectionStatus('connected', 'Connected successfully!');
        } else if (state === 'disconnected') {
          this.setConnectionStatus('disconnected', 'Disconnected from server');
          if (this.currentUser && this.config.onDisconnect) {
            this.config.onDisconnect();
          }
        }
      }
    });

    try {
      await this.client.connect();
    } catch (error) {
      this.setConnectionStatus('error', error instanceof Error ? error.message : 'Connection failed');
      throw error;
    }
  }

  /**
   * Public method to connect to server (for initial connection)
   */
  async connect(): Promise<void> {
    return this.ensureConnected();
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

    // Handle authentication responses
    const pending = this.pendingRequests.get(packetType);
    if (pending) {
      this.pendingRequests.delete(packetType);

      try {
        let response: any;
        switch (packetType) {
          case PACKET_TYPE.LOGIN_RESPONSE:
            response = decodeLoginResponse(packet.data);
            console.log('Login response decoded:', response);
            break;
          case PACKET_TYPE.SIGNUP_RESPONSE:
            response = decodeSignupResponse(packet.data);
            console.log('Signup response decoded:', response);
            break;
          case PACKET_TYPE.PACKET_TABLES:
            response = decodeTableListResponse(packet.data);
            console.log('Table list response decoded:', response);
            break;
          case PACKET_TYPE.CREATE_TABLE_RESPONSE:
            response = decodeGenericResponse(packet.data);
            console.log('Create table response decoded:', response);
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
    console.error('AuthService error:', error);
    // Reject all pending requests
    this.pendingRequests.forEach((pending) => {
      pending.reject(error);
    });
    this.pendingRequests.clear();
  }

  /**
   * Login with username and password
   */
  async login(username: string, password: string): Promise<User> {
    await this.ensureConnected();

    return new Promise<User>((resolve, reject) => {
      try {
        // Encode login request
        const payload = encodeLoginRequest(username, password);
        const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.LOGIN_REQUEST, payload);

        // Register pending request
        this.pendingRequests.set(PACKET_TYPE.LOGIN_RESPONSE, {
          resolve: (response: LoginResponse) => {
            if (response.result === 0 && response.user_id) {
              // Login successful - convert server response to User object
              const user: User = {
                user_id: response.user_id,
                username: response.username || '',
                email: response.email || '',
                phone: response.phone || '',
                full_name: response.fullname || '',
                country: response.country || '',
                gender: response.gender || '',
                dob: response.dob || '',
                balance: response.balance || 0,
                avatar_url: response.avatar_url || ''
              };
              this.currentUser = user;
              resolve(user);
            } else {
              // Login failed
              reject(new Error(response.msg || 'Login failed'));
            }
          },
          reject
        });

        // Send packet
        this.client!.send(packet.data);

        // Set timeout
        setTimeout(() => {
          if (this.pendingRequests.has(PACKET_TYPE.LOGIN_RESPONSE)) {
            this.pendingRequests.delete(PACKET_TYPE.LOGIN_RESPONSE);
            reject(new Error('Login request timeout'));
          }
        }, 10000);
      } catch (error) {
        reject(error instanceof Error ? error : new Error(String(error)));
      }
    });
  }

  /**
   * Sign up a new user
   */
  async signup(request: SignupRequest): Promise<void> {
    await this.ensureConnected();

    return new Promise<void>((resolve, reject) => {
      try {
        // Encode signup request
        const payload = encodeSignupRequest(request);
        const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.SIGNUP_REQUEST, payload);

        // Register pending request
        this.pendingRequests.set(PACKET_TYPE.SIGNUP_RESPONSE, {
          resolve: (response: SignupResponse) => {
            if (response.res === 201) {
              // Signup successful
              resolve();
            } else {
              // Signup failed
              reject(new Error(response.msg || 'Signup failed'));
            }
          },
          reject
        });

        // Send packet
        this.client!.send(packet.data);

        // Set timeout
        setTimeout(() => {
          if (this.pendingRequests.has(PACKET_TYPE.SIGNUP_RESPONSE)) {
            this.pendingRequests.delete(PACKET_TYPE.SIGNUP_RESPONSE);
            reject(new Error('Signup request timeout'));
          }
        }, 10000);
      } catch (error) {
        reject(error instanceof Error ? error : new Error(String(error)));
      }
    });
  }

  /**
   * Logout and disconnect
   */
  logout(): void {
    this.currentUser = null;
    if (this.client) {
      this.client.disconnect();
      this.client = null;
    }
    this.pendingRequests.clear();
    this.setConnectionStatus('disconnected', 'Logged out');
  }

  /**
   * Get current user
   */
  getCurrentUser(): User | null {
    return this.currentUser;
  }

  /**
   * Check if user is logged in
   */
  isLoggedIn(): boolean {
    return this.currentUser !== null;
  }

  /**
   * Get the TCP client (for sending game packets)
   */
  getClient(): TcpClient | null {
    return this.client;
  }

  /**
   * Send raw packet
   */
  sendPacket(data: Uint8Array): void {
    if (!this.client || !this.client.isConnected()) {
      throw new Error('Not connected to server');
    }
    this.client.send(data);
  }

  /**
   * Get all tables from server
   */
  async getTables(): Promise<TableListResponse> {
    await this.ensureConnected();

    return new Promise<TableListResponse>((resolve, reject) => {
      try {
        // Encode GET_TABLES request (empty payload)
        const payload = encodeGetTablesRequest();
        const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.PACKET_TABLES, payload);

        // Register pending request
        this.pendingRequests.set(PACKET_TYPE.PACKET_TABLES, {
          resolve: (response: TableListResponse) => {
            resolve(response);
          },
          reject
        });

        // Send packet
        this.client!.send(packet.data);

        // Set timeout
        setTimeout(() => {
          if (this.pendingRequests.has(PACKET_TYPE.PACKET_TABLES)) {
            this.pendingRequests.delete(PACKET_TYPE.PACKET_TABLES);
            reject(new Error('Get tables request timeout'));
          }
        }, 10000);
      } catch (error) {
        reject(error instanceof Error ? error : new Error(String(error)));
      }
    });
  }

  /**
   * Create a new table
   */
  async createTable(request: CreateTableRequest): Promise<void> {
    await this.ensureConnected();

    return new Promise<void>((resolve, reject) => {
      try {
        // Encode create table request
        const payload = encodeCreateTableRequest(request);
        const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.CREATE_TABLE_REQUEST, payload);

        // Register pending request
        this.pendingRequests.set(PACKET_TYPE.CREATE_TABLE_RESPONSE, {
          resolve: (response: GenericResponse) => {
            if (response.res === PACKET_TYPE.R_CREATE_TABLE_OK) {
              // Table created successfully
              resolve();
            } else {
              // Table creation failed
              reject(new Error(response.msg || 'Failed to create table'));
            }
          },
          reject
        });

        // Send packet
        this.client!.send(packet.data);

        // Set timeout
        setTimeout(() => {
          if (this.pendingRequests.has(PACKET_TYPE.CREATE_TABLE_RESPONSE)) {
            this.pendingRequests.delete(PACKET_TYPE.CREATE_TABLE_RESPONSE);
            reject(new Error('Create table request timeout'));
          }
        }, 10000);
      } catch (error) {
        reject(error instanceof Error ? error : new Error(String(error)));
      }
    });
  }

  /**
   * Set connection status and notify listeners
   */
  private setConnectionStatus(status: ConnectionStatus, message?: string): void {
    this.connectionStatus = status;
    if (this.config.onConnectionStatusChange) {
      this.config.onConnectionStatusChange(status, message);
    }
  }

  /**
   * Get current connection status
   */
  getConnectionStatus(): ConnectionStatus {
    return this.connectionStatus;
  }
}
