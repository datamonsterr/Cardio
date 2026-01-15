// Card Types
export type Suit = 'Heart' | 'Spade' | 'Club' | 'Diamond'
export type CardFace = '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' | '10' | 'J' | 'Q' | 'K' | 'A'

export interface Card {
  cardFace: CardFace
  suit: Suit
  value: number
  animationDelay?: number
}

// User Types
export interface User {
  id: string
  username: string
  password?: string // Optional since we remove it after auth
  email: string
  chips: number
  level?: number
  gamesPlayed: number
  wins: number
  avatarURL?: string
  totalWinnings?: number
}

// Player Types
export interface Player {
  name: string
  chips: number
  position: number
  cards?: Card[]
  bet?: number
  folded?: boolean
  allIn?: boolean
  isActive?: boolean
  showCards?: boolean
  avatarURL?: string
  startChips?: number
  endChips?: number
}

// Table Types
export interface Table {
  id: string
  name: string
  minBet: number
  maxPlayers: number
  currentPlayers: number
  status: 'active' | 'waiting' | 'full'
  players: Player[]
}

// Game Phase Types
export type GamePhase =
  | 'loading'
  | 'initialDeal'
  | 'betting1'
  | 'flop'
  | 'betting2'
  | 'turn'
  | 'betting3'
  | 'river'
  | 'betting4'
  | 'showdown'

// Game State Types
export interface GameState {
  players: Player[]
  activePlayerIndex: number
  dealerIndex: number
  blindIndex: number
  phase: GamePhase
  pot: number
  highBet: number
  betInputValue: number
  communityCards: Card[]
  deck: Card[]
  sidePots: SidePot[]
  showDownMessages: ShowDownMessage[]
}

export interface SidePot {
  amount: number
  players: string[]
}

export interface ShowDownMessage {
  users: string[]
  prize: number
  rank: string
}

// Auth Context Types
export interface AuthContextType {
  user: User | null
  login: (username: string, password: string) => Promise<boolean>
  logout: () => void
  updateChips: (amount: number) => void
  loading: boolean
  signup?: (request: SignupRequestType) => Promise<void>
  getTables?: () => Promise<any>
  createTable?: (request: CreateTableRequestType) => Promise<number>
  getClient?: () => any
}

// Create table request type
export interface CreateTableRequestType {
  name: string
  max_player: number
  min_bet: number
}

// Signup request type
export interface SignupRequestType {
  user: string
  pass: string
  email: string
  phone: string
  fullname: string
  country: string
  gender: string
  dob: string
}

// Slider Types
export interface SliderHandle {
  id: string
  value: number
  percent: number
}

export interface SliderTrack {
  source: SliderHandle
  target: SliderHandle
}
