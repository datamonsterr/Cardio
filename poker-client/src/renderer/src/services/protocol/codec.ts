/**
 * Protocol packet encoding/decoding functions
 * Mirrors the C server implementation using MessagePack
 */

import { encode as msgpackEncode, decode as msgpackDecode } from '@msgpack/msgpack';
import {
  Header,
  Packet,
  RawBytes,
  LoginRequest,
  LoginResponse,
  SignupRequest,
  SignupResponse,
  GenericResponse,
  TableListResponse,
  CreateTableRequest,
  GameState,
  ActionRequest,
  ActionResult,
  JoinTableRequest
} from './types';
import { HEADER_SIZE } from './constants';

/**
 * Encode a packet header and payload into raw bytes
 * Matches encode_packet() in protocol.c
 */
export function encodePacket(
  protocolVer: number,
  packetType: number,
  payload: Uint8Array | null
): RawBytes {
  const payloadLen = payload ? payload.length : 0;
  const totalLen = HEADER_SIZE + payloadLen;

  // Allocate buffer for header + payload
  const buffer = new Uint8Array(totalLen);
  const view = new DataView(buffer.buffer);

  // Write header (5 bytes)
  // packet_len (2 bytes, big-endian)
  view.setUint16(0, totalLen, false); // false = big-endian

  // protocol_ver (1 byte)
  view.setUint8(2, protocolVer);

  // packet_type (2 bytes, big-endian)
  view.setUint16(3, packetType, false); // false = big-endian

  // Copy payload if present
  if (payload && payloadLen > 0) {
    buffer.set(payload, HEADER_SIZE);
  }

  return {
    data: buffer,
    len: totalLen
  };
}

/**
 * Decode packet header from raw bytes
 * Matches decode_header() in protocol.c
 */
export function decodeHeader(data: Uint8Array): Header {
  if (data.length < HEADER_SIZE) {
    throw new Error(`Insufficient data for header: ${data.length} bytes`);
  }

  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);

  return {
    packet_len: view.getUint16(0, false),    // false = big-endian
    protocol_ver: view.getUint8(2),
    packet_type: view.getUint16(3, false)    // false = big-endian
  };
}

/**
 * Decode complete packet from raw bytes
 * Matches decode_packet() in protocol.c
 */
export function decodePacket(data: Uint8Array): Packet {
  if (data.length < HEADER_SIZE) {
    throw new Error(`Insufficient data for packet: ${data.length} bytes`);
  }

  const header = decodeHeader(data);

  // Validate packet length
  if (header.packet_len > data.length) {
    throw new Error(
      `Packet length mismatch: header=${header.packet_len}, actual=${data.length}`
    );
  }

  // Extract payload
  const payloadLen = header.packet_len - HEADER_SIZE;
  const payload = data.slice(HEADER_SIZE, HEADER_SIZE + payloadLen);

  return {
    header,
    data: payload
  };
}

/**
 * Encode login request payload
 * Matches decode_login_request() format in protocol.c
 */
export function encodeLoginRequest(username: string, password: string): Uint8Array {
  const payload: LoginRequest = {
    user: username,
    pass: password
  };
  return new Uint8Array(msgpackEncode(payload));
}

/**
 * Decode login response payload
 */
export function decodeLoginResponse(data: Uint8Array): LoginResponse {
  return msgpackDecode(data) as LoginResponse;
}

/**
 * Encode signup request payload
 * Matches decode_signup_request() format in protocol.c
 */
export function encodeSignupRequest(req: SignupRequest): Uint8Array {
  return new Uint8Array(msgpackEncode(req));
}

/**
 * Decode signup response payload
 */
export function decodeSignupResponse(data: Uint8Array): SignupResponse {
  return msgpackDecode(data) as SignupResponse;
}

/**
 * Decode generic response (res + optional msg)
 * Matches encode_response() and encode_response_msg() in protocol.c
 */
export function decodeGenericResponse(data: Uint8Array): GenericResponse {
  return msgpackDecode(data) as GenericResponse;
}

/**
 * Decode table list response payload (PACKET_TABLES = 500)
 * Matches encode_full_tables_response() in server/src/protocol.c
 */
export function decodeTableListResponse(data: Uint8Array): TableListResponse {
  return msgpackDecode(data) as TableListResponse;
}

/**
 * Free packet resources (for consistency with C API)
 * In TypeScript, this is mostly a no-op since GC handles cleanup
 */
export function freePacket(_packet: Packet): void {
  // No-op in TypeScript - garbage collector handles cleanup
}

/**
 * Helper to create handshake request
 * Matches the handshake protocol format
 */
export function createHandshakeRequest(protocolVersion: number): Uint8Array {
  const buffer = new Uint8Array(4);
  const view = new DataView(buffer.buffer);

  // length (2 bytes) - always 0x0002
  view.setUint16(0, 2, false);

  // protocol_version (2 bytes)
  view.setUint16(2, protocolVersion, false);

  return buffer;
}

/**
 * Parse handshake response
 */
export function parseHandshakeResponse(data: Uint8Array): { code: number } {
  if (data.length < 3) {
    throw new Error(`Invalid handshake response: ${data.length} bytes`);
  }

  const view = new DataView(data.buffer, data.byteOffset);
  const length = view.getUint16(0, false);
  const code = view.getUint8(2);

  if (length !== 1) {
    throw new Error(`Invalid handshake response length: ${length}`);
  }

  return { code };
}

/**
 * Encode GET_TABLES request (empty payload)
 * Packet ID: 500
 */
export function encodeGetTablesRequest(): Uint8Array {
  // GET_TABLES request has empty payload
  return new Uint8Array(0);
}

/**
 * Encode create table request payload
 * Matches decode_create_table_request() format in server/src/protocol.c
 * Server expects: { "name": string, "max_player": int, "min_bet": int }
 */
export function encodeCreateTableRequest(req: CreateTableRequest): Uint8Array {
  return new Uint8Array(msgpackEncode(req));
}

/**
 * Encode join table request payload
 * Matches decode_join_table_request() format in server/src/protocol.c
 * Server expects: { "tableId": int }
 */
export function encodeJoinTableRequest(req: JoinTableRequest): Uint8Array {
  const payload = { tableId: req.table_id };
  return new Uint8Array(msgpackEncode(payload));
}

/**
 * Decode game state from server
 * Matches encode_game_state() format in server/src/protocol_game.c
 */
export function decodeGameState(data: Uint8Array): GameState {
  return msgpackDecode(data) as GameState;
}

/**
 * Encode action request payload
 * Matches decode_action_request() format in server/src/protocol_game.c
 * Server expects: { "game_id": int, "action": { "type": string, "amount": int }, "client_seq": uint32 }
 */
export function encodeActionRequest(req: ActionRequest): Uint8Array {
  const payload = {
    game_id: req.game_id,
    action: {
      type: req.action.type,
      ...(req.action.amount !== undefined && { amount: req.action.amount })
    },
    client_seq: req.client_seq
  };
  return new Uint8Array(msgpackEncode(payload));
}

/**
 * Decode action result response
 * Matches encode_action_result() format in server/src/protocol_game.c
 */
export function decodeActionResult(data: Uint8Array): ActionResult {
  return msgpackDecode(data) as ActionResult;
}
