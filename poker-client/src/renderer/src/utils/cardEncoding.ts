/**
 * Card encoding/decoding utilities
 * Matches server card encoding: suit * 13 + (rank - 2)
 * Suits: 1=Spades, 2=Hearts, 3=Diamonds, 4=Clubs
 * Ranks: 2-14 (where 14=Ace)
 */

import type { Card, Suit, CardFace } from '../types';

const SUIT_NAMES: Suit[] = ['Spade', 'Heart', 'Diamond', 'Club'];
const CARD_FACES: CardFace[] = ['2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A'];

const VALUE_MAP: Record<CardFace, number> = {
  '2': 1,
  '3': 2,
  '4': 3,
  '5': 4,
  '6': 5,
  '7': 6,
  '8': 7,
  '9': 8,
  '10': 9,
  'J': 10,
  'Q': 11,
  'K': 12,
  'A': 13,
};

/**
 * Decode server card integer to client Card format
 * @param encodedCard Server card encoding (-1 for hidden, or suit*13 + (rank-2))
 * @returns Card object or null if hidden
 */
export function decodeCard(encodedCard: number): Card | null {
  if (encodedCard === -1) {
    return null; // Hidden card
  }

  // Decode: suit * 13 + (rank - 2)
  const suitIndex = Math.floor((encodedCard - 1) / 13);
  const rankValue = ((encodedCard - 1) % 13) + 2;

  if (suitIndex < 0 || suitIndex >= SUIT_NAMES.length) {
    console.error(`Invalid suit index: ${suitIndex} from card ${encodedCard}`);
    return null;
  }

  if (rankValue < 2 || rankValue > 14) {
    console.error(`Invalid rank value: ${rankValue} from card ${encodedCard}`);
    return null;
  }

  const suit = SUIT_NAMES[suitIndex];
  const cardFace = CARD_FACES[rankValue - 2];
  const value = VALUE_MAP[cardFace];

  return {
    cardFace,
    suit,
    value
  };
}

/**
 * Encode client Card to server integer format
 * @param card Client Card object
 * @returns Server card encoding integer
 */
export function encodeCard(card: Card): number {
  const suitIndex = SUIT_NAMES.indexOf(card.suit);
  const rankValue = VALUE_MAP[card.cardFace];
  const rank = rankValue === 13 ? 14 : rankValue; // Ace is 14
  
  return (suitIndex + 1) * 13 + (rank - 2);
}

/**
 * Decode array of server card integers
 * @param encodedCards Array of server card encodings
 * @returns Array of Card objects (null for hidden cards)
 */
export function decodeCards(encodedCards: number[]): (Card | null)[] {
  return encodedCards.map(decodeCard);
}
