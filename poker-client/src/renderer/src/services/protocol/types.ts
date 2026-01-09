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

// Action request
export interface ActionRequest {
  game_id: number;
  action_type: string;  // "fold", "check", "call", "bet", "raise"
  amount?: number;      // for bet/raise
  client_seq: number;
}

// Action result
export interface ActionResult {
  result: number;       // 0=OK, 400=BAD_ACTION, 403=NOT_YOUR_TURN, etc.
  client_seq: number;
  reason?: string;
}

// Player state in game
export interface PlayerState {
  user_id: number;
  username: string;
  seat: number;
  chips: number;
  bet: number;
  state: string;        // "active", "folded", "all_in", "empty"
  cards: number[];      // [-1, -1] for hidden, or actual card values
}

// Pot information
export interface Pot {
  amount: number;
  eligible_seats: number[];
}

// Game state
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
  players: PlayerState[];
  community_cards: number[];
  main_pot: Pot;
  side_pots: Pot[];
  current_bet: number;
  min_raise: number;
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

// Create table request
export interface CreateTableRequest {
  name: string;
  min_bet: number;
  max_bet: number;
  max_players: number;
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
