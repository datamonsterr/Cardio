/**
 * Protocol types matching the C server implementation
 */

// Header structure (5 bytes)
export interface Header {
  packet_len: number;    // uint16 - Total packet length including header
  protocol_ver: number;  // uint8 - Protocol version
  packet_type: number;   // uint16 - Packet type identifier
}

// Complete packet structure
export interface Packet {
  header: Header;
  data: Uint8Array;     // Raw payload bytes
}

// Raw bytes container
export interface RawBytes {
  data: Uint8Array;
  len: number;
}

// Login request payload
export interface LoginRequest {
  user: string;         // username
  pass: string;         // password
}

// Login response payload
export interface LoginResponse {
  result: number;       // 0 = success, non-zero = error
  user_id?: number;
  username?: string;
  email?: string;
  phone?: string;
  fullname?: string;
  country?: string;
  gender?: string;
  dob?: string;
  balance?: number;
  avatar_url?: string;
  msg?: string;         // optional error message
}

// Signup request payload
export interface SignupRequest {
  user: string;         // username
  pass: string;         // password
  email: string;
  phone: string;
  fullname: string;
  country: string;
  gender: string;
  dob: string;          // YYYY-MM-DD format
}

// Signup response payload
export interface SignupResponse {
  res: number;          // response code
  msg?: string;         // optional message
}

// Generic response
export interface GenericResponse {
  res: number;
  msg?: string;
}

// Action request (matches server decode_action_request format)
export interface ActionRequest {
  game_id: number;
  action: {
    type: string;       // "fold", "check", "call", "bet", "raise", "all_in"
    amount?: number;    // for bet/raise
  };
  client_seq: number;
}

// Action result
export interface ActionResult {
  result: number;       // 0=OK, 400=BAD_ACTION, 403=NOT_YOUR_TURN, etc.
  client_seq: number;
  reason?: string;
}

// Player state in game (matches server protocol_game.c)
export interface PlayerState {
  player_id: number;
  name: string;
  seat: number;
  state: string;        // "empty", "waiting", "active", "folded", "all_in", "sitting_out"
  money: number;        // Stack size
  bet: number;          // Current bet this round
  total_bet: number;    // Total bet this hand
  cards: number[];      // [-1, -1] for hidden, or actual card values (encoded integers)
  is_dealer: boolean;
  is_small_blind: boolean;
  is_big_blind: boolean;
  timer_deadline?: number;  // Epoch_ms when action expires
}

// Pot information (matches server protocol)
export interface Pot {
  amount: number;
  eligible_players: number[];  // Array of player_ids
}

// Side pot information
export interface SidePot {
  amount: number;
  eligible_players: number[];
}

// Available action (for active player)
export interface AvailableAction {
  type: string;         // "fold", "check", "call", "bet", "raise", "all_in"
  min_amount: number;
  max_amount: number;
  increment: number;
}

// Game state (matches server encode_game_state format)
export interface GameState {
  game_id: number;
  hand_id: number;
  seq: number;
  max_players: number;
  small_blind: number;
  big_blind: number;
  min_buy_in: number;
  max_buy_in: number;
  betting_round: string;  // "preflop", "flop", "turn", "river", "showdown", "complete"
  dealer_seat: number;
  active_seat: number;
  players: (PlayerState | null)[];  // Array of MAX_PLAYERS (9), null for empty seats
  community_cards: number[];        // Array of encoded card integers (0-5 cards)
  main_pot: number;                 // Main pot amount
  side_pots: SidePot[];             // Array of side pots
  current_bet: number;
  min_raise: number;
  available_actions?: AvailableAction[];  // Only present for active player
}

// Table info (matches server response format)
export interface TableInfo {
  id: number;
  tableName: string;
  maxPlayer: number;
  minBet: number;
  currentPlayer: number;
}

// Table list response (matches encode_full_tables_response format)
export interface TableListResponse {
  size: number;
  tables: TableInfo[];
}

// Create table request (matches server protocol - decode_create_table_request)
export interface CreateTableRequest {
  name: string;        // Max 32 chars (server expects "name" key)
  max_player: number; // 2-9 (server expects "max_player" key)
  min_bet: number;    // Minimum bet (big blind) (server expects "min_bet" key)
}

// Join table request
export interface JoinTableRequest {
  table_id: number;
}

// Leave table request
export interface LeaveTableRequest {
  table_id: number;
}

// ===== Lobby / Tables =====

// Matches server/src/protocol.c encode_full_tables_response()
export interface ServerTableSummary {
  id: number;
  tableName: string;
  maxPlayer: number;
  minBet: number;
  currentPlayer: number;
}

export interface FullTablesResponse {
  size: number;
  tables: ServerTableSummary[];
}
