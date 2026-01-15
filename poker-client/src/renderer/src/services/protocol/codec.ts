/**
 * Protocol packet encoding/decoding functions
 * Mirrors the C server implementation using MessagePack
 */

import { encode as msgpackEncode, decode as msgpackDecode } from '@msgpack/msgpack'
import {
  Header,
  Packet,
  RawBytes,
  LoginRequest,
  LoginResponse,
  SignupRequest,
  SignupResponse,
  GenericResponse,
  CreateTableResponse,
  BalanceUpdateNotification,
  TableListResponse,
  CreateTableRequest
} from './types'
import { HEADER_SIZE } from './constants'

/**
 * Encode a packet header and payload into raw bytes
 * Matches encode_packet() in protocol.c
 */
export function encodePacket(
  protocolVer: number,
  packetType: number,
  payload: Uint8Array | null
): RawBytes {
  const payloadLen = payload ? payload.length : 0
  const totalLen = HEADER_SIZE + payloadLen

  // Allocate buffer for header + payload
  const buffer = new Uint8Array(totalLen)
  const view = new DataView(buffer.buffer)

  // Write header (5 bytes)
  // packet_len (2 bytes, big-endian)
  view.setUint16(0, totalLen, false) // false = big-endian

  // protocol_ver (1 byte)
  view.setUint8(2, protocolVer)

  // packet_type (2 bytes, big-endian)
  view.setUint16(3, packetType, false) // false = big-endian

  // Copy payload if present
  if (payload && payloadLen > 0) {
    buffer.set(payload, HEADER_SIZE)
  }

  return {
    data: buffer,
    len: totalLen
  }
}

/**
 * Decode packet header from raw bytes
 * Matches decode_header() in protocol.c
 */
export function decodeHeader(data: Uint8Array): Header {
  if (data.length < HEADER_SIZE) {
    throw new Error(`Insufficient data for header: ${data.length} bytes`)
  }

  const view = new DataView(data.buffer, data.byteOffset, data.byteLength)

  return {
    packet_len: view.getUint16(0, false), // false = big-endian
    protocol_ver: view.getUint8(2),
    packet_type: view.getUint16(3, false) // false = big-endian
  }
}

/**
 * Decode complete packet from raw bytes
 * Matches decode_packet() in protocol.c
 */
export function decodePacket(data: Uint8Array): Packet {
  if (data.length < HEADER_SIZE) {
    throw new Error(`Insufficient data for packet: ${data.length} bytes`)
  }

  const header = decodeHeader(data)

  // Validate packet length
  if (header.packet_len > data.length) {
    throw new Error(`Packet length mismatch: header=${header.packet_len}, actual=${data.length}`)
  }

  // Extract payload
  const payloadLen = header.packet_len - HEADER_SIZE
  const payload = data.slice(HEADER_SIZE, HEADER_SIZE + payloadLen)

  return {
    header,
    data: payload
  }
}

/**
 * Encode login request payload
 * Matches decode_login_request() format in protocol.c
 */
export function encodeLoginRequest(username: string, password: string): Uint8Array {
  const payload: LoginRequest = {
    user: username,
    pass: password
  }
  return new Uint8Array(msgpackEncode(payload))
}

/**
 * Decode login response payload
 */
export function decodeLoginResponse(data: Uint8Array): LoginResponse {
  return msgpackDecode(data) as LoginResponse
}

/**
 * Encode signup request payload
 * Matches decode_signup_request() format in protocol.c
 */
export function encodeSignupRequest(req: SignupRequest): Uint8Array {
  return new Uint8Array(msgpackEncode(req))
}

/**
 * Decode signup response payload
 */
export function decodeSignupResponse(data: Uint8Array): SignupResponse {
  return msgpackDecode(data) as SignupResponse
}

/**
 * Decode generic response (res + optional msg)
 * Matches encode_response() and encode_response_msg() in protocol.c
 */
export function decodeGenericResponse(data: Uint8Array): GenericResponse {
  return msgpackDecode(data) as GenericResponse
}

/**
 * Decode create table response (PACKET_TYPE = 300)
 * Matches encode_create_table_response() in server/src/protocol.c
 */
export function decodeCreateTableResponse(data: Uint8Array): CreateTableResponse {
  return msgpackDecode(data) as CreateTableResponse
}

/**
 * Decode balance update notification (PACKET_BALANCE_UPDATE = 970)
 * Matches encode_balance_update_notification() in server/src/protocol.c
 */
export function decodeBalanceUpdateNotification(data: Uint8Array): BalanceUpdateNotification {
  return msgpackDecode(data) as BalanceUpdateNotification
}

/**
 * Decode table list response payload (PACKET_TABLES = 500)
 * Matches encode_full_tables_response() in server/src/protocol.c
 */
export function decodeTableListResponse(data: Uint8Array): TableListResponse {
  return msgpackDecode(data) as TableListResponse
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
  const buffer = new Uint8Array(4)
  const view = new DataView(buffer.buffer)

  // length (2 bytes) - always 0x0002
  view.setUint16(0, 2, false)

  // protocol_version (2 bytes)
  view.setUint16(2, protocolVersion, false)

  return buffer
}

/**
 * Parse handshake response
 */
export function parseHandshakeResponse(data: Uint8Array): { code: number } {
  if (data.length < 3) {
    throw new Error(`Invalid handshake response: ${data.length} bytes`)
  }

  const view = new DataView(data.buffer, data.byteOffset)
  const length = view.getUint16(0, false)
  const code = view.getUint8(2)

  if (length !== 1) {
    throw new Error(`Invalid handshake response length: ${length}`)
  }

  return { code }
}

/**
 * Encode GET_TABLES request (empty payload)
 * Packet ID: 500
 */
export function encodeGetTablesRequest(): Uint8Array {
  // GET_TABLES request has empty payload
  return new Uint8Array(0)
}

/**
 * Encode create table request payload
 * Matches decode_create_table_request() format in server/src/protocol.c
 * Server expects: { "name": string, "max_player": int, "min_bet": int }
 */
export function encodeCreateTableRequest(req: CreateTableRequest): Uint8Array {
  return new Uint8Array(msgpackEncode(req))
}

// ===== Friend Management Codec Functions =====

/**
 * Encode add friend request
 * Matches decode_add_friend_request() in server/src/protocol.c
 * Server expects: { "username": string }
 */
export function encodeAddFriendRequest(username: string): Uint8Array {
  return new Uint8Array(msgpackEncode({ username }))
}

/**
 * Encode invite friend request
 * Matches decode_invite_friend_request() in server/src/protocol.c
 * Server expects: { "username": string }
 */
export function encodeInviteFriendRequest(username: string): Uint8Array {
  return new Uint8Array(msgpackEncode({ username }))
}

/**
 * Encode accept/reject invite request
 * Matches decode_invite_action_request() in server/src/protocol.c
 * Server expects: { "invite_id": int }
 */
export function encodeInviteActionRequest(inviteId: number): Uint8Array {
  return new Uint8Array(msgpackEncode({ invite_id: inviteId }))
}

/**
 * Encode get invites request (empty payload)
 */
export function encodeGetInvitesRequest(): Uint8Array {
  return new Uint8Array(0)
}

/**
 * Decode generic friend response
 * Server sends: { "res": int, "msg"?: string }
 */
export function decodeFriendResponse(data: Uint8Array): GenericResponse {
  return msgpackDecode(data) as GenericResponse
}

/**
 * Decode get invites response
 * Server sends array of: [{ "invite_id": int, "from_user_id": int, "from_username": string, "status": string, "created_at": string }, ...]
 */
export function decodeGetInvitesResponse(data: Uint8Array): Array<{
  invite_id: number
  from_user_id: number
  from_username: string
  status: string
  created_at: string
}> {
  return msgpackDecode(data) as Array<{
    invite_id: number
    from_user_id: number
    from_username: string
    status: string
    created_at: string
  }>
}

/**
 * Decode friend list response
 * Server sends array of: [{ "user_id": int, "username": string }, ...]
 */
export function decodeFriendListResponse(data: Uint8Array): Array<{
  user_id: number
  username: string
}> {
  return msgpackDecode(data) as Array<{
    user_id: number
    username: string
  }>
}

// ===== Game Action Codec Functions =====

/**
 * Encode action request
 * Matches decode_action_request() in server/src/protocol.c
 * Server expects: { "game_id": int, "action": { "type": string, "amount"?: int }, "client_seq": int }
 */
export function encodeActionRequest(request: {
  game_id: number
  action_type: string
  amount?: number
  client_seq: number
}): Uint8Array {
  const payload = {
    game_id: request.game_id,
    action: {
      type: request.action_type,
      ...(request.amount !== undefined && request.amount > 0 ? { amount: request.amount } : {})
    },
    client_seq: request.client_seq
  }
  return new Uint8Array(msgpackEncode(payload))
}

/**
 * Decode action result
 * Server sends: { "result": int, "client_seq": int, "reason"?: string }
 */
export function decodeActionResult(data: Uint8Array): {
  result: number
  client_seq: number
  reason?: string
} {
  return msgpackDecode(data) as {
    result: number
    client_seq: number
    reason?: string
  }
}

/**
 * Decode game state
 * Server sends full game state with all fields
 */
export function decodeGameState(data: Uint8Array): any {
  return msgpackDecode(data)
}

/**
 * Encode join table request
 * Server expects: { "table_id": int }
 */
export function encodeJoinTableRequest(tableId: number): Uint8Array {
  return new Uint8Array(msgpackEncode({ table_id: tableId }))
}

/**
 * Encode leave table request
 * Server expects: { "table_id": int }
 */
export function encodeLeaveTableRequest(tableId: number): Uint8Array {
  return new Uint8Array(msgpackEncode({ table_id: tableId }))
}

/**
 * Encode table invite request (packet 980)
 * Server expects: { "friend_username": string, "table_id": int }
 */
export function encodeTableInviteRequest(friendUsername: string, tableId: number): Uint8Array {
  return new Uint8Array(
    msgpackEncode({
      friend_username: friendUsername,
      table_id: tableId
    })
  )
}

/**
 * Decode table invite response
 * Server sends: { "result": int, "message": string }
 * Response codes:
 * - 981: R_INVITE_TO_TABLE_OK
 * - 982: R_INVITE_TO_TABLE_NOT_OK
 * - 983: R_INVITE_TO_TABLE_NOT_FRIENDS
 * - 984: R_INVITE_TO_TABLE_ALREADY_IN_GAME
 */
export interface TableInviteResponse {
  result: number
  message: string
}

export function decodeTableInviteResponse(data: Uint8Array): TableInviteResponse {
  const decoded = msgpackDecode(data) as { res?: number; result?: number; message?: string }
  return {
    result: decoded.result || decoded.res || 0,
    message: decoded.message || 'Unknown response'
  }
}

/**
 * Decode incoming table invite notification (packet 985)
 * Server sends: { "from_user": string, "table_id": int, "table_name": string }
 */
export interface TableInviteNotificationData {
  fromUser: string
  tableId: number
  tableName: string
}

export function decodeTableInviteNotification(
  data: Uint8Array
): TableInviteNotificationData {
  const decoded = msgpackDecode(data) as {
    from_user: string
    table_id: number
    table_name: string
  }
  return {
    fromUser: decoded.from_user,
    tableId: decoded.table_id,
    tableName: decoded.table_name
  }
}
