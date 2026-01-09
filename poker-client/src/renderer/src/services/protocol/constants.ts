/**
 * Protocol constants matching the C server implementation
 */

// Protocol version
export const PROTOCOL_V1 = 0x01;

// Packet types
export const PACKET_TYPE = {
  // Service
  PING: 10,
  PONG: 11,

  // Authentication
  LOGIN_REQUEST: 100,
  LOGIN_RESPONSE: 100,
  R_LOGIN_OK: 101,
  R_LOGIN_NOT_OK: 102,

  // Registration
  SIGNUP_REQUEST: 200,
  SIGNUP_RESPONSE: 200,
  R_SIGNUP_OK: 201,
  R_SIGNUP_NOT_OK: 202,

  // Table Management
  CREATE_TABLE_REQUEST: 300,
  CREATE_TABLE_RESPONSE: 300,
  R_CREATE_TABLE_OK: 301,
  R_CREATE_TABLE_NOT_OK: 302,
  PACKET_TABLES: 500,

  // Game Flow
  JOIN_TABLE_REQUEST: 400,
  JOIN_TABLE_RESPONSE: 400,
  R_JOIN_TABLE_OK: 401,
  R_JOIN_TABLE_NOT_OK: 402,
  R_JOIN_TABLE_FULL: 403,
  ACTION_REQUEST: 450,
  ACTION_RESULT: 451,
  UPDATE_BUNDLE: 460,
  RESYNC_REQUEST: 470,
  RESYNC_RESPONSE: 471,

  // Game State
  UPDATE_GAMESTATE: 600,

  // Leave Table
  LEAVE_TABLE_REQUEST: 700,
  LEAVE_TABLE_RESPONSE: 700,

  // Scoreboard
  GET_SCOREBOARD: 800,
  SCOREBOARD_RESPONSE: 800,

  // Friend List
  GET_FRIENDLIST: 810,
  FRIENDLIST_RESPONSE: 810
} as const;

// Response codes
export const RESPONSE_CODE = {
  R_LOGIN_OK: 101,
  R_LOGIN_NOT_OK: 102,
  R_SIGNUP_OK: 201,
  R_SIGNUP_NOT_OK: 202,
  R_CREATE_TABLE_OK: 301,
  R_CREATE_TABLE_NOT_OK: 302,
  R_JOIN_TABLE_OK: 401,
  R_JOIN_TABLE_NOT_OK: 402,
  R_JOIN_TABLE_FULL: 403
} as const;

// Header size
export const HEADER_SIZE = 5; // 2 (len) + 1 (ver) + 2 (type)
