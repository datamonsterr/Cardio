import type { Table } from '../types';

// Mock data for poker tables
export const mockTables: Table[] = [
  {
    id: 'table-1',
    name: 'Table 1',
    minBet: 50,
    maxPlayers: 6,
    currentPlayers: 4,
    status: 'active',
    players: [
      { name: 'Alice', chips: 5000, position: 0 },
      { name: 'Bob', chips: 3500, position: 1 },
      { name: 'Charlie', chips: 7200, position: 3 },
      { name: 'Diana', chips: 4100, position: 5 },
    ],
  },
  {
    id: 'table-2',
    name: 'Table 2',
    minBet: 5,
    maxPlayers: 9,
    currentPlayers: 7,
    status: 'active',
    players: [
      { name: 'Eve', chips: 850, position: 0 },
      { name: 'Frank', chips: 420, position: 1 },
      { name: 'Grace', chips: 650, position: 2 },
      { name: 'Henry', chips: 1100, position: 4 },
      { name: 'Ivy', chips: 380, position: 5 },
      { name: 'Jack', chips: 920, position: 6 },
      { name: 'Kate', chips: 560, position: 8 },
    ],
  },
  {
    id: 'table-3',
    name: 'Table 3',
    minBet: 25,
    maxPlayers: 6,
    currentPlayers: 2,
    status: 'waiting',
    players: [
      { name: 'Leo', chips: 2500, position: 0 },
      { name: 'Mia', chips: 3000, position: 3 },
    ],
  },
  {
    id: 'table-4',
    name: 'Table 4',
    minBet: 250,
    maxPlayers: 6,
    currentPlayers: 6,
    status: 'full',
    players: [
      { name: 'Noah', chips: 15000, position: 0 },
      { name: 'Olivia', chips: 22000, position: 1 },
      { name: 'Paul', chips: 8500, position: 2 },
      { name: 'Quinn', chips: 18000, position: 3 },
      { name: 'Rachel', chips: 30000, position: 4 },
      { name: 'Sam', chips: 12000, position: 5 },
    ],
  },
  {
    id: 'table-5',
    name: 'Table 5',
    minBet: 1,
    maxPlayers: 9,
    currentPlayers: 5,
    status: 'active',
    players: [
      { name: 'Tom', chips: 150, position: 1 },
      { name: 'Uma', chips: 85, position: 3 },
      { name: 'Victor', chips: 190, position: 4 },
      { name: 'Wendy', chips: 120, position: 6 },
      { name: 'Xavier', chips: 95, position: 8 },
    ],
  },
  {
    id: 'table-6',
    name: 'Table 6',
    minBet: 100,
    maxPlayers: 9,
    currentPlayers: 3,
    status: 'active',
    players: [
      { name: 'Yara', chips: 8500, position: 2 },
      { name: 'Zack', chips: 12000, position: 5 },
      { name: 'Amy', chips: 6500, position: 7 },
    ],
  },
];

interface Achievement {
  name: string;
  icon: string;
  unlocked: boolean;
}

interface UserStats {
  totalGames: number;
  gamesWon: number;
  totalWinnings: number;
  winRate: number;
  favoriteGame: string;
  currentStreak: number;
  bestHand: string;
  achievements: Achievement[];
}

// Mock user statistics
export const mockUserStats: UserStats = {
  totalGames: 156,
  gamesWon: 68,
  totalWinnings: 45600,
  winRate: 43.6,
  favoriteGame: "Texas Hold'em",
  currentStreak: 3,
  bestHand: 'Royal Flush',
  achievements: [
    { name: 'First Win', icon: '🏆', unlocked: true },
    { name: 'High Roller', icon: '💎', unlocked: true },
    { name: 'Tournament King', icon: '👑', unlocked: false },
    { name: 'Straight Flush', icon: '🔥', unlocked: true },
    { name: '100 Games', icon: '🎯', unlocked: true },
  ],
};

interface RecentGame {
  id: string;
  date: string;
  table: string;
  result: 'Won' | 'Lost';
  profit: number;
  duration: string;
}

// Mock recent games
export const mockRecentGames: RecentGame[] = [
  {
    id: 'game-1',
    date: '2026-01-08',
    table: 'High Rollers',
    result: 'Won',
    profit: 850,
    duration: '45 min',
  },
  {
    id: 'game-2',
    date: '2026-01-08',
    table: 'Beginners Paradise',
    result: 'Won',
    profit: 320,
    duration: '32 min',
  },
  {
    id: 'game-3',
    date: '2026-01-07',
    table: 'Quick Play',
    result: 'Lost',
    profit: -500,
    duration: '28 min',
  },
  {
    id: 'game-4',
    date: '2026-01-07',
    table: 'VIP Room',
    result: 'Won',
    profit: 2400,
    duration: '68 min',
  },
  {
    id: 'game-5',
    date: '2026-01-06',
    table: 'Tournament Final',
    result: 'Lost',
    profit: -1200,
    duration: '92 min',
  },
];
