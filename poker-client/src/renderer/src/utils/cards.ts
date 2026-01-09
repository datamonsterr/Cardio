import type { Card, Suit, CardFace } from '../types';

const totalNumCards = 52;
const suits: Suit[] = ['Heart', 'Spade', 'Club', 'Diamond'];
const cards: CardFace[] = ['2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A'];

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

const randomizePosition = (min: number, max: number): number => {
  min = Math.ceil(min);
  max = Math.floor(max);
  return Math.floor(Math.random() * (max - min + 1)) + min;
};

export const generateDeckOfCards = (): Card[] => {
  const deck: Card[] = [];

  for (const suit of suits) {
    for (const card of cards) {
      deck.push({
        cardFace: card,
        suit: suit,
        value: VALUE_MAP[card],
      });
    }
  }
  return deck;
};

export const shuffle = (deck: Card[]): Card[] => {
  const shuffledDeck: (Card | undefined)[] = new Array(totalNumCards);
  const filledSlots: number[] = [];
  
  for (let i = 0; i < totalNumCards; i++) {
    if (i === 51) {
      // Fill last undefined slot when only 1 card left to shuffle
      const lastSlot = shuffledDeck.findIndex((el) => typeof el === 'undefined');
      if (lastSlot !== -1) {
        shuffledDeck[lastSlot] = deck[i];
        filledSlots.push(lastSlot);
      }
    } else {
      let shuffleToPosition = randomizePosition(0, totalNumCards - 1);
      while (filledSlots.includes(shuffleToPosition)) {
        shuffleToPosition = randomizePosition(0, totalNumCards - 1);
      }
      shuffledDeck[shuffleToPosition] = deck[i];
      filledSlots.push(shuffleToPosition);
    }
  }
  return shuffledDeck.filter((card): card is Card => card !== undefined);
};

export const popCards = (deck: Card[], numToPop: number): { 
  mutableDeckCopy: Card[]; 
  chosenCards: Card | Card[] 
} => {
  const mutableDeckCopy = [...deck];
  let chosenCards: Card | Card[];
  
  if (numToPop === 1) {
    chosenCards = mutableDeckCopy.pop()!;
  } else {
    chosenCards = [];
    for (let i = 0; i < numToPop; i++) {
      const card = mutableDeckCopy.pop();
      if (card) {
        chosenCards.push(card);
      }
    }
  }
  
  return { mutableDeckCopy, chosenCards };
};

export const dealCards = (deck: Card[], numPlayers: number): {
  deck: Card[];
  playerCards: Card[][];
} => {
  let mutableDeck = [...deck];
  const playerCards: Card[][] = Array.from({ length: numPlayers }, () => []);

  // Deal 2 cards to each player
  for (let i = 0; i < 2; i++) {
    for (let j = 0; j < numPlayers; j++) {
      const result = popCards(mutableDeck, 1);
      mutableDeck = result.mutableDeckCopy;
      playerCards[j].push(result.chosenCards as Card);
    }
  }

  return { deck: mutableDeck, playerCards };
};

export const dealFlop = (deck: Card[]): { deck: Card[]; flopCards: Card[] } => {
  const result = popCards(deck, 3);
  return {
    deck: result.mutableDeckCopy,
    flopCards: result.chosenCards as Card[],
  };
};

export const dealTurnOrRiver = (deck: Card[]): { deck: Card[]; card: Card } => {
  const result = popCards(deck, 1);
  return {
    deck: result.mutableDeckCopy,
    card: result.chosenCards as Card,
  };
};
