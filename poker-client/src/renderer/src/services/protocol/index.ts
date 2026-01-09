/**
 * Protocol module exports
 */

export * from './constants';
export * from './types';
export * from './codec';

// Re-export commonly used functions
export {
  encodePacket,
  decodePacket,
  decodeHeader,
  encodeLoginRequest,
  encodeSignupRequest,
  encodeGetTablesRequest,
  encodeCreateTableRequest,
  encodeJoinTableRequest,
  encodeActionRequest,
  decodeLoginResponse,
  decodeSignupResponse,
  decodeTableListResponse,
  decodeGenericResponse,
  decodeGameState,
  decodeActionResult,
  createHandshakeRequest,
  parseHandshakeResponse
} from './codec';
