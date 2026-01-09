import type { Player } from '../types';

export const determineMinBet = (
  highBet: number,
  playerChipsStack: number,
  playerBet: number
): number => {
  const playerTotalChips = playerChipsStack + playerBet;
  if (playerTotalChips < highBet) {
    return playerTotalChips;
  } else {
    return highBet;
  }
};

export const determineBlindIndices = (
  dealerIndex: number,
  numPlayers: number
): { bigBlindIndex: number; smallBlindIndex: number } => {
  return {
    bigBlindIndex: (dealerIndex + 2) % numPlayers,
    smallBlindIndex: (dealerIndex + 1) % numPlayers,
  };
};

export const anteUpBlinds = (
  players: Player[],
  blindIndices: { bigBlindIndex: number; smallBlindIndex: number },
  minBet: number
): Player[] => {
  const { bigBlindIndex, smallBlindIndex } = blindIndices;
  const updatedPlayers = [...players];
  
  updatedPlayers[bigBlindIndex] = {
    ...updatedPlayers[bigBlindIndex],
    bet: minBet,
    chips: updatedPlayers[bigBlindIndex].chips - minBet,
  };
  
  updatedPlayers[smallBlindIndex] = {
    ...updatedPlayers[smallBlindIndex],
    bet: minBet / 2,
    chips: updatedPlayers[smallBlindIndex].chips - minBet / 2,
  };
  
  return updatedPlayers;
};
