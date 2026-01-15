/**
 * Convert server GameState format to client GameState format
 */

import type { GameState as ServerGameState, PlayerState } from '../services/protocol';
import type { Player, Card, GamePhase } from '../types';
import { decodeCard } from './cardEncoding';

/**
 * Convert server betting_round string to client GamePhase
 */
function convertBettingRound(round: string): GamePhase {
  switch (round) {
    case 'preflop':
      return 'betting1';
    case 'flop':
      return 'flop';
    case 'turn':
      return 'turn';
    case 'river':
      return 'river';
    case 'showdown':
      return 'showdown';
    case 'complete':
      return 'showdown';
    default:
      return 'loading';
  }
}

/**
 * Convert server PlayerState to client Player format
 */
function convertPlayer(
  serverPlayer: PlayerState | null,
  userPlayerId: number,
  index: number
): Player | null {
  if (!serverPlayer) {
    return null;
  }

  // Decode cards - only show if it's the user's player or during showdown
  const cards: Card[] = [];
  if (serverPlayer.cards && serverPlayer.cards.length >= 2) {
    const card1 = decodeCard(serverPlayer.cards[0]);
    const card2 = decodeCard(serverPlayer.cards[1]);
    if (card1 && card2) {
      cards.push(card1, card2);
    }
  }

  return {
    name: serverPlayer.name,
    chips: serverPlayer.money,
    position: serverPlayer.seat,
    cards: cards.length > 0 ? cards : undefined,
    bet: serverPlayer.bet,
    folded: serverPlayer.state === 'folded',
    allIn: serverPlayer.state === 'all_in',
    isActive: serverPlayer.state === 'active',
    showCards: serverPlayer.state === 'folded' ? false : undefined,
    avatarURL: `https://api.dicebear.com/7.x/avataaars/svg?seed=${serverPlayer.name}`,
  };
}

/**
 * Convert server GameState to client GameState format
 */
export function convertServerGameState(
  serverState: ServerGameState,
  userPlayerId: number
): {
  players: Player[];
  activePlayerIndex: number;
  dealerIndex: number;
  communityCards: Card[];
  pot: number;
  highBet: number;
  betInputValue: number;
  minBet: number;
  phase: GamePhase;
} {
  // Convert players array - filter out null entries and map to client format
  const players: Player[] = [];
  let activePlayerIndex = -1;
  let dealerIndex = -1;

  serverState.players.forEach((serverPlayer, index) => {
    if (serverPlayer) {
      const player = convertPlayer(serverPlayer, userPlayerId, index);
      if (player) {
        players.push(player);
        
        if (serverPlayer.seat === serverState.active_seat) {
          activePlayerIndex = players.length - 1;
        }
        if (serverPlayer.seat === serverState.dealer_seat) {
          dealerIndex = players.length - 1;
        }
      }
    }
  });

  // Decode community cards
  const communityCards: Card[] = [];
  serverState.community_cards.forEach((encodedCard) => {
    const card = decodeCard(encodedCard);
    if (card) {
      communityCards.push(card);
    }
  });

  // Convert betting round to phase
  const phase = convertBettingRound(serverState.betting_round);

  return {
    players,
    activePlayerIndex: activePlayerIndex >= 0 ? activePlayerIndex : 0,
    dealerIndex: dealerIndex >= 0 ? dealerIndex : 0,
    communityCards,
    pot: serverState.main_pot,
    highBet: serverState.current_bet,
    betInputValue: serverState.min_raise || serverState.big_blind,
    minBet: serverState.big_blind,
    phase
  };
}
