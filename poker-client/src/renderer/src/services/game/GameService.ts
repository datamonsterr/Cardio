/**
 * GameService - Handles game-specific communication with server
 * Manages game state updates, player actions, and game events
 */

import { TcpClient } from '../network/TcpClient';
import { encodeActionRequest, decodeGameState, decodeActionResult, encodePacket } from '../protocol/codec';
import { PACKET_UPDATE_GAMESTATE, PACKET_UPDATE_BUNDLE, PACKET_ACTION_REQUEST, PROTOCOL_V1 } from '../protocol/constants';
import type { GameState, ActionResult, Packet } from '../protocol/types';

export type GameEventType = 'stateUpdate' | 'actionResult' | 'error';

export interface GameEventHandler {
  (type: GameEventType, data: any): void;
}

export interface GameServiceConfig {
  tcpClient: TcpClient;
  onGameEvent?: GameEventHandler;
}

export class GameService {
  private client: TcpClient;
  private eventHandler?: GameEventHandler;
  private currentGameState: GameState | null = null;

  constructor(config: GameServiceConfig) {
    this.client = config.tcpClient;
    this.eventHandler = config.onGameEvent;

    // Subscribe to packets from TcpClient
    this.client = new TcpClient({
      ...config.tcpClient['config'], // Access the original config
      onPacket: this.handlePacket.bind(this),
      onError: (error) => this.emit('error', error),
    });
  }

  /**
   * Handle incoming packets from server
   */
  private handlePacket(packet: Packet): void {
    switch (packet.header.packet_type) {
      case PACKET_UPDATE_GAMESTATE:
        this.handleGameStateUpdate(packet);
        break;
      case PACKET_UPDATE_BUNDLE:
        this.handleUpdateBundle(packet);
        break;
      default:
        // Ignore other packet types
        break;
    }
  }

  /**
   * Handle game state update
   */
  private handleGameStateUpdate(packet: Packet): void {
    try {
      const gameState = decodeGameState(packet.data);
      this.currentGameState = gameState;
      this.emit('stateUpdate', gameState);
    } catch (error) {
      console.error('Failed to decode game state:', error);
      this.emit('error', new Error('Failed to decode game state'));
    }
  }

  /**
   * Handle update bundle (game state + notifications)
   */
  private handleUpdateBundle(packet: Packet): void {
    // For now, treat it like a game state update
    // TODO: Parse bundle format properly
    this.handleGameStateUpdate(packet);
  }

  /**
   * Send a player action to the server
   */
  async sendAction(
    gameId: number,
    actionType: 'fold' | 'check' | 'call' | 'bet' | 'raise' | 'all_in',
    amount?: number
  ): Promise<ActionResult> {
    return new Promise((resolve, reject) => {
      const clientSeq = Date.now() & 0xffffffff;

      // Encode action payload
      const payload = encodeActionRequest({
        game_id: gameId,
        action_type: actionType,
        amount: amount || 0,
        client_seq: clientSeq,
      });

      // Wrap payload in packet with header
      const packet = encodePacket(PROTOCOL_V1, PACKET_ACTION_REQUEST, payload);

      // Listen for action result
      const originalOnPacket = this.client['config'].onPacket;
      const timeoutId = setTimeout(() => {
        this.client['config'].onPacket = originalOnPacket;
        reject(new Error('Action timeout'));
      }, 5000);

      this.client['config'].onPacket = (packet: Packet) => {
        // Pass through to handlePacket
        if (originalOnPacket) originalOnPacket(packet);

        // Check if this is our action result
        if (packet.header.packet_type === 451) {
          // PACKET_ACTION_RESULT
          try {
            const result = decodeActionResult(packet.data);
            if (result.client_seq === clientSeq) {
              clearTimeout(timeoutId);
              this.client['config'].onPacket = originalOnPacket;
              
              if (result.result === 0) {
                resolve(result);
              } else {
                reject(new Error(result.reason || `Action failed with code ${result.result}`));
              }
            }
          } catch (error) {
            clearTimeout(timeoutId);
            this.client['config'].onPacket = originalOnPacket;
            reject(error);
          }
        }
      };

      // Send the action packet
      this.client.send(packet.data);
    });
  }

  /**
   * Get current game state
   */
  getGameState(): GameState | null {
    return this.currentGameState;
  }

  /**
   * Check if it's the player's turn
   */
  isMyTurn(userId: number): boolean {
    if (!this.currentGameState) return false;
    
    const activePlayer = this.currentGameState.players.find(
      p => p.seat === this.currentGameState!.active_seat
    );
    
    return activePlayer?.user_id === userId;
  }

  /**
   * Get player's current chips
   */
  getPlayerChips(userId: number): number {
    if (!this.currentGameState) return 0;
    
    const player = this.currentGameState.players.find(p => p.user_id === userId);
    return player?.chips || 0;
  }

  /**
   * Get minimum raise amount
   */
  getMinRaise(): number {
    return this.currentGameState?.min_raise || 0;
  }

  /**
   * Get current bet amount
   */
  getCurrentBet(): number {
    return this.currentGameState?.current_bet || 0;
  }

  /**
   * Check if player can perform action
   */
  canPerformAction(userId: number, action: string): boolean {
    if (!this.isMyTurn(userId)) return false;

    const playerChips = this.getPlayerChips(userId);
    const currentBet = this.getCurrentBet();
    const player = this.currentGameState?.players.find(p => p.user_id === userId);
    const playerBet = player?.bet || 0;

    switch (action) {
      case 'fold':
        return true;
      case 'check':
        return playerBet === currentBet;
      case 'call':
        return playerBet < currentBet && playerChips > 0;
      case 'bet':
      case 'raise':
        return playerChips > (currentBet - playerBet);
      case 'all_in':
        return playerChips > 0;
      default:
        return false;
    }
  }

  /**
   * Emit event to handler
   */
  private emit(type: GameEventType, data: any): void {
    if (this.eventHandler) {
      this.eventHandler(type, data);
    }
  }

  /**
   * Clean up resources
   */
  destroy(): void {
    this.currentGameState = null;
    this.eventHandler = undefined;
  }
}
