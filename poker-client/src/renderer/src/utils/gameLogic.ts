import axios from 'axios';
import type { Player } from '../types';

const generateRandomChips = (): number => {
  return Math.floor(Math.random() * (20000 - 15000)) + 15000;
};

export const generateTable = async (): Promise<Player[]> => {
  const players: Player[] = [
    {
      name: 'Player 1',
      avatarURL: new URL('../assets/boy.svg', import.meta.url).href,
      cards: [],
      chips: 20000,
      position: 0,
      bet: 0,
      folded: false,
      allIn: false,
      isActive: false,
      startChips: 20000,
      endChips: 20000,
    },
  ];

  try {
    // Fetch 4 random users for AI players with avatars
    const response = await axios.get('https://randomuser.me/api/?results=4&nat=us,gb,fr');
    
    response.data.results.forEach((user: any, i: number) => {
      const chips = generateRandomChips();
      const firstName = user.name.first.charAt(0).toUpperCase() + user.name.first.slice(1);
      const lastName = user.name.last.charAt(0).toUpperCase() + user.name.last.slice(1);
      
      players.push({
        name: `${firstName} ${lastName}`,
        avatarURL: user.picture.large,
        cards: [],
        chips,
        position: i + 1,
        bet: 0,
        folded: false,
        allIn: false,
        isActive: false,
        startChips: chips,
        endChips: chips,
      });
    });
  } catch (error) {
    console.error('Failed to fetch avatars, using fallback:', error);
    // Fallback to local avatars if API fails
    const fallbackNames = ['Alice Smith', 'Bob Johnson', 'Charlie Brown', 'Diana Prince'];
    for (let i = 0; i < 4; i++) {
      const chips = generateRandomChips();
      players.push({
        name: fallbackNames[i],
        avatarURL: new URL('../assets/boy.svg', import.meta.url).href,
        cards: [],
        chips,
        position: i + 1,
        bet: 0,
        folded: false,
        allIn: false,
        isActive: false,
        startChips: chips,
        endChips: chips,
      });
    }
  }

  return players;
};

export const checkWin = (players: Player[]): boolean => {
  const playersWithChips = players.filter(player => player.chips > 0);
  return playersWithChips.length === 1;
};

export const beginNextRound = (state: any): any => {
  // Implementation for next round logic
  // This is a simplified version - full implementation would need more game logic
  return {
    ...state,
    phase: 'initialDeal',
    pot: 0,
    communityCards: [],
    highBet: state.minBet,
  };
};
