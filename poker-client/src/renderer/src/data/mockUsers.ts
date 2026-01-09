import type { User } from '../types';

// Mock user credentials
export const mockUsers: User[] = [
  {
    id: 'user-1',
    username: 'player1',
    password: 'password123',
    email: 'player1@poker.com',
    chips: 10000,
    level: 5,
    gamesPlayed: 42,
    wins: 18,
    avatarURL: 'https://randomuser.me/api/portraits/men/1.jpg',
  },
  {
    id: 'user-2',
    username: 'pokerpro',
    password: 'poker2024',
    email: 'pokerpro@poker.com',
    chips: 25000,
    level: 12,
    gamesPlayed: 156,
    wins: 89,
    avatarURL: 'https://randomuser.me/api/portraits/women/2.jpg',
  },
  {
    id: 'user-3',
    username: 'admin',
    password: 'admin',
    email: 'admin@poker.com',
    chips: 100000,
    level: 20,
    gamesPlayed: 500,
    wins: 280,
    avatarURL: 'https://randomuser.me/api/portraits/men/3.jpg',
  },
  {
    id: 'user-4',
    username: 'beginner',
    password: 'beginner123',
    email: 'beginner@poker.com',
    chips: 1000,
    level: 1,
    gamesPlayed: 5,
    wins: 1,
    avatarURL: 'https://randomuser.me/api/portraits/women/4.jpg',
  },
];

// Find user by credentials
export function authenticateUser(username: string, password: string): User | null {
  const user = mockUsers.find(
    (u) => u.username === username && u.password === password
  );
  return user || null;
}

// Get user by ID
export function getUserById(id: string): User | null {
  return mockUsers.find((u) => u.id === id) || null;
}
