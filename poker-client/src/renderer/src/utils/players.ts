import type { Player } from '../types';

export const handleOverflowIndex = (index: number, numPlayers: number): number => {
  return index >= numPlayers ? index % numPlayers : index;
};

export const determinePhaseStartActivePlayer = (
  dealerIndex: number,
  numPlayers: number
): number => {
  return (dealerIndex + 3) % numPlayers;
};

export const findNextActivePlayer = (
  players: Player[],
  currentIndex: number
): number => {
  let nextIndex = (currentIndex + 1) % players.length;
  
  while (
    nextIndex !== currentIndex &&
    (players[nextIndex].folded || players[nextIndex].allIn)
  ) {
    nextIndex = (nextIndex + 1) % players.length;
  }
  
  return nextIndex;
};

export const getActivePlayers = (players: Player[]): Player[] => {
  return players.filter((player) => !player.folded && !player.allIn);
};

export const allButOnePlayedFolded = (players: Player[]): boolean => {
  const activePlayers = players.filter((player) => !player.folded);
  return activePlayers.length === 1;
};
