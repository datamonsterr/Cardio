# Nuoa Game Server Protocol Documentation

## Overview

This document describes the complete protocol specification for the Cardio poker game server. The protocol is designed to be:
- **Authoritative**: Server maintains game state
- **Efficient**: Binary format with MessagePack serialization
- **Versioned**: Supports protocol evolution
- **Resilient**: Handles disconnects, timeouts, and resyncs

## Table of Contents

1. [Transport and Framing](#transport-and-framing)
2. [Versioned Handshake](#versioned-handshake)
3. [Packet Structure](#packet-structure)
4. [Packet ID Layout](#packet-id-layout)
5. [Service Packets](#service-packets)
6. [Authentication](#authentication)
7. [Lobby Management](#lobby-management)
8. [Game Join/Leave](#game-joinleave)
9. [In-Game Updates](#in-game-updates)
10. [Player Actions](#player-actions)
11. [Resynchronization](#resynchronization)
12. [Error Handling](#error-handling)
13. [Example Flows](#example-flows)

---

## Transport and Framing

### Transport Layer
- **Protocol**: Persistent TCP (TLS-TCP recommended for production)
- **Byte Order**: Big-endian (network byte order)
- **Authority**: Server is authoritative

### Message Framing

All application messages use the following frame structure:

```
┌─────────────┬──────────────┬──────────────┬─────────────┐
│ packet_len  │ protocol_ver │ packet_type  │  payload    │
│  (2 bytes)  │  (1 byte)    │  (2 bytes)   │  (variable) │
└─────────────┴──────────────┴──────────────┴─────────────┘
```

- **packet_len** (u16): Total packet size including header (5 bytes + payload)
- **protocol_ver** (u8): Protocol version (currently 0x01)
- **packet_type** (u16): Packet identifier
- **payload**: MessagePack-encoded data (unless specified otherwise)

### Serialization
- **Format**: MessagePack for all payloads
- **Field naming**: lowercase with underscores (snake_case)
- **Unknown fields**: MUST be ignored by receivers
- **Timestamps**: Unix epoch milliseconds (u64)

### Timeouts and Keepalive
- **Idle timeout**: Server closes connection after 60s of inactivity
- **Handshake timeout**: 5s to complete handshake
- **Turn timeout**: Configurable per game (typically 20-30s)
- **Heartbeat**: Optional PING/PONG every 20-30s

---

## Versioned Handshake

The handshake occurs before any regular packets to negotiate protocol version.

### Client → Server (Handshake Request)

```
┌─────────────┬──────────────────┐
│   length    │ protocol_version │
│  (2 bytes)  │    (2 bytes)     │
└─────────────┴──────────────────┘
```

- **length**: Always 0x0002 (2 bytes for version field)
- **protocol_version**: Requested version (e.g., 0x0001 for v1)

### Server → Client (Handshake Response)

```
┌─────────────┬──────────┐
│   length    │   code   │
│  (2 bytes)  │ (1 byte) │
└─────────────┴──────────┘
```

**Response Codes:**
- `0x00`: HANDSHAKE_OK - Protocol version accepted
- `0x01`: PROTOCOL_NOT_SUPPORTED - Version not supported
- `0x02`: SERVER_FULL - Server at capacity (v2+)

**Example (hex):**
- Client request v1: `00 02 00 01`
- Server accepts: `00 01 00`

---

## Packet Structure

### Current Protocol (v1)

After successful handshake, all messages follow the standard packet format described in Transport and Framing.

**Header Structure (5 bytes):**
```c
typedef struct __attribute__((packed)) {
    uint16_t packet_len;      // Total packet length (big-endian)
    uint8_t  protocol_ver;    // Protocol version (0x01)
    uint16_t packet_type;     // Packet ID (big-endian)
} Header;
```

---

## Packet ID Layout

Packet IDs are organized by functionality:

| Range     | Direction | Purpose                          |
|-----------|-----------|----------------------------------|
| 1-19      | Both      | Service (PING/PONG, errors)      |
| 100-199   | Both      | Authentication                   |
| 200-299   | Both      | User registration                |
| 300-399   | Both      | Table/Lobby management           |
| 400-499   | Both      | Game join/leave + state sync     |
| 500-699   | Both      | In-game actions                  |
| 700-799   | Both      | Leave table                      |
| 800-899   | Both      | Scoreboard                       |
| 900-999   | S2C       | Out-of-band errors/alerts        |

### Assigned Packet IDs

**Service:**
- `10`: PING (C2S or S2C)
- `11`: PONG (C2S or S2C)

**Authentication:**
- `100`: LOGIN_REQUEST (C2S) / LOGIN_RESPONSE (S2C)
- `101`: R_LOGIN_OK (S2C)
- `102`: R_LOGIN_NOT_OK (S2C)

**Registration:**
- `200`: SIGNUP_REQUEST (C2S) / SIGNUP_RESPONSE (S2C)
- `201`: R_SIGNUP_OK (S2C)
- `202`: R_SIGNUP_NOT_OK (S2C)

**Table Management:**
- `300`: CREATE_TABLE_REQUEST (C2S) / CREATE_TABLE_RESPONSE (S2C)
- `301`: R_CREATE_TABLE_OK (S2C)
- `302`: R_CREATE_TABLE_NOT_OK (S2C)
- `500`: PACKET_TABLES - Get all tables (C2S/S2C)

**Game Flow:**
- `400`: JOIN_TABLE_REQUEST (C2S) / JOIN_TABLE_RESPONSE (S2C)
- `401`: R_JOIN_TABLE_OK (S2C)
- `402`: R_JOIN_TABLE_NOT_OK (S2C)
- `403`: R_JOIN_TABLE_FULL (S2C)
- `450`: ACTION_REQUEST (C2S) - Player action
- `451`: ACTION_RESULT (S2C) - Action acknowledgment
- `460`: UPDATE_BUNDLE (S2C) - State updates
- `470`: RESYNC_REQUEST (C2S)
- `471`: RESYNC_RESPONSE (S2C)

**Game State:**
- `600`: PACKET_UPDATE_GAMESTATE (S2C) - Full state sync (currently sent after each action)

**Leave Table:**
- `700`: LEAVE_TABLE_REQUEST (C2S)
- `701`: R_LEAVE_TABLE_OK (S2C)
- `702`: R_LEAVE_TABLE_NOT_OK (S2C)

**Social Features:**
- `800`: PACKET_SCOREBOARD (C2S/S2C)
- `801`: R_SCOREBOARD_OK (S2C)
- `802`: R_SCOREBOARD_NOT_OK (S2C)
- `900`: PACKET_FRIENDLIST (C2S/S2C)
- `901`: R_FRIENDLIST_OK (S2C)
- `902`: R_FRIENDLIST_NOT_OK (S2C)

**Errors:**
- `900`: ERROR (S2C) - Generic error message

---

## Service Packets

### PING (ID: 10)

**Direction**: Bidirectional

**Payload** (MessagePack):
```json
{
  "t": <epoch_ms>
}
```

### PONG (ID: 11)

**Direction**: Bidirectional

**Payload** (MessagePack):
```json
{
  "t": <epoch_ms>
}
```

### ERROR (ID: 900)

**Direction**: S2C

**Payload** (MessagePack):
```json
{
  "code": <int>,
  "message": "<string>",
  "context": <object>  // Optional
}
```

**Standard Error Codes:**
- `400`: BAD_REQUEST
- `401`: UNAUTHORIZED
- `403`: FORBIDDEN
- `404`: NOT_FOUND
- `409`: CONFLICT
- `422`: UNPROCESSABLE
- `429`: RATE_LIMIT
- `500`: SERVER_ERROR
- `503`: UNAVAILABLE

---

## Authentication

### LOGIN_REQUEST (ID: 100)

**Direction**: C2S

**Payload** (MessagePack):
```json
{
  "user": "<string>",      // Username (max 32 chars)
  "password": "<string>"   // Password (max 32 chars)
}
```

**C Implementation:**
```c
typedef struct {
    char username[32];
    char password[32];
} LoginRequest;
```

### LOGIN_RESPONSE (ID: 100)

**Direction**: S2C

**Response Types**: All responses use packet type 100. The R_* constants are result codes in the payload.

- Result `101` (R_LOGIN_OK): Login successful  
- Result `102` (R_LOGIN_NOT_OK): Login failed

**Payload on Success** (MessagePack):
```json
{
  "result": 0,
  "user_id": <int>,
  "username": "<string>",
  "balance": <int>,
  "fullname": "<string>",
  "email": "<string>"
}
```

**Payload on Failure** (MessagePack):
```json
{
  "result": <error_code>,
  "message": "<error_message>"
}
```

**Result Codes:**
- `0`: LOGIN_OK
- `1`: NO_SUCH_USER
- `2`: INVALID_PASSWORD
- `-1`: SERVER_ERROR

### SIGNUP_REQUEST (ID: 200)

**Direction**: C2S

**Payload** (MessagePack):
```json
{
  "username": "<string>",  // Max 32 chars
  "password": "<string>",  // Max 32 chars
  "fullname": "<string>",  // Max 64 chars
  "email": "<string>",     // Max 64 chars
  "phone": "<string>",     // Max 16 chars
  "dob": "<string>",       // Date of birth (max 16 chars)
  "country": "<string>",   // Max 32 chars
  "gender": "<string>"     // Max 8 chars
}
```

### SIGNUP_RESPONSE (ID: 200)

**Direction**: S2C

**Response Types**: All responses use packet type 200. The R_* constants are result codes in the payload.

- Result `201` (R_SIGNUP_OK): Registration successful
- Result `202` (R_SIGNUP_NOT_OK): Registration failed

**Result Codes:**
- `0`: REGISTER_OK
- `1`: USERNAME_IN_USE
- `2`: ILLEGAL_USERNAME
- `3`: ILLEGAL_PASSWORD
- `-1`: SERVER_ERROR

---

## Lobby Management

### CREATE_TABLE_REQUEST (ID: 300)

**Direction**: C2S

**Payload** (MessagePack):
```json
{
  "table_name": "<string>",  // Max 32 chars
  "max_player": <int>,       // 2-9
  "min_bet": <int>           // Minimum bet (big blind)
}
```

### CREATE_TABLE_RESPONSE (ID: 300)

**Direction**: S2C

**Response Types**: All responses use packet type 300. The R_* constants are result codes in the payload.

- Result `301` (R_CREATE_TABLE_OK): Table created
- Result `302` (R_CREATE_TABLE_NOT_OK): Failed to create

### GET_TABLES (ID: 500)

**Direction**: C2S → S2C

**Request Payload**: Empty

**Response Payload** (MessagePack):
```json
{
  "tables": [
    {
      "id": <int>,
      "name": "<string>",
      "current_player": <int>,
      "max_player": <int>,
      "min_bet": <int>,
      "max_bet": <int>
    },
    ...
  ]
}
```

---

## Game Join/Leave

### JOIN_TABLE_REQUEST (ID: 400)

**Direction**: C2S

**Payload** (MessagePack):
```json
{
  "table_id": <int>
}
```

### JOIN_TABLE_RESPONSE (ID: 400)

**Direction**: S2C

**Note**: All responses use packet type 400 (PACKET_JOIN_TABLE). The constants 401-403 are result codes in the MessagePack payload, not packet types.

**Response Payload**:

On success, server sends either:
1. Simple success response:
```json
{
  "res": 401  // R_JOIN_TABLE_OK
}
```

2. Full game state (when game is active):
```json
{
  "game_id": <int>,
  "hand_id": <int>,
  // ... (see GAME_STATE structure below)
}
```

On failure:
```json
{
  "res": 402  // R_JOIN_TABLE_NOT_OK or 403 for R_JOIN_TABLE_FULL
}
```

### LEAVE_TABLE_REQUEST (ID: 700)

**Direction**: C2S

**Payload** (MessagePack):
```json
{
  "table_id": <int>
}
```

### LEAVE_TABLE_RESPONSE (ID: 700)

**Direction**: S2C

**Response Types**: All responses use packet type 700. The R_* constants are result codes in the payload.

- Result `701` (R_LEAVE_TABLE_OK): Successfully left
- Result `702` (R_LEAVE_TABLE_NOT_OK): Leave failed

---

## Game State Structure

### GAME_STATE (Full Sync)

This is sent when a player joins a table or requests a resync.

```json
{
  "game_id": <int>,
  "hand_id": <int>,
  "seq": <u32>,
  
  "max_players": <int>,
  "small_blind": <int>,
  "big_blind": <int>,
  "min_buy_in": <int>,
  "max_buy_in": <int>,
  
  "betting_round": "<string>",  // "preflop","flop","turn","river","showdown","complete"
  "dealer_seat": <int>,
  "active_seat": <int>,
  
  "players": [           // Array of MAX_PLAYERS (9) elements
    null,                  // Empty seats are represented as null
    {
      "player_id": <int>,
      "name": "<string>",
      "seat": <int>,
      "state": "<string>",      // "empty","waiting","active","folded","all_in","sitting_out"
      "money": <int>,           // Stack size
      "bet": <int>,             // Current bet this round
      "total_bet": <int>,       // Total bet this hand
      "cards": [<int>, <int>],  // -1 for hidden cards, player only sees own cards
      "is_dealer": <bool>,
      "is_small_blind": <bool>,
      "is_big_blind": <bool>,
      "timer_deadline": <u64>   // Epoch_ms when action expires
    },
    ...
  ],
  
  "community_cards": [<int>, ...],  // 0-5 cards
  
  "main_pot": <int>,
  "side_pots": [
    {
      "amount": <int>,
      "eligible_players": [<int>, ...]
    },
    ...
  ],
  
  "current_bet": <int>,
  "min_raise": <int>,
  
  "available_actions": [
    {
      "type": "<string>",       // "fold","check","call","bet","raise","all_in"
      "min_amount": <int>,
      "max_amount": <int>,
      "increment": <int>
    },
    ...
  ]
}
```

### Card Encoding

Cards are represented as integers:
- **-1**: Hidden/unknown card
- **Valid cards**: `suit * 13 + (rank - 2)`
  - Suits: 1=Spades, 2=Hearts, 3=Diamonds, 4=Clubs
  - Ranks: 2-14 (where 14=Ace)

**Examples:**
- 2 of Spades: `1 * 13 + 0 = 13`
- Ace of Hearts: `2 * 13 + 12 = 38`

---

## In-Game Updates

### UPDATE_BUNDLE (ID: 460)

**Direction**: S2C

**Implementation Note**: Current implementation sends full game state via PACKET_UPDATE_GAMESTATE (600) after each action instead of incremental updates. UPDATE_BUNDLE with delta updates is planned for future optimization.

Game state changes are sent as atomic bundles containing notifications (for UI/animations) and updates (for state synchronization).

**Payload** (MessagePack):
```json
{
  "seq": <u32>,              // Monotonically increasing sequence number
  "notifications": [
    {
      "type": "<string>",
      // ... notification-specific fields
    },
    ...
  ],
  "updates": [
    {
      "type": "<string>",
      // ... update-specific fields
    },
    ...
  ]
}
```

### Update Types

**State Updates** (modify client model):

- `HAND_STARTED`: New hand beginning
  ```json
  {"type": "HAND_STARTED", "hand_id": <int>}
  ```

- `BETTING_ROUND`: Round changed
  ```json
  {"type": "BETTING_ROUND", "round": "<string>"}
  ```

- `ACTIVE_PLAYER`: Turn changed
  ```json
  {"type": "ACTIVE_PLAYER", "player_id": <int>, "seat": <int>}
  ```

- `PLAYER_TIMER`: Action timer set
  ```json
  {"type": "PLAYER_TIMER", "player_id": <int>, "timer": <u64>}
  ```

- `PLAYER_STATE`: Player state changed
  ```json
  {"type": "PLAYER_STATE", "player_id": <int>, "state": "<string>"}
  ```

- `PLAYER_MONEY`: Stack updated
  ```json
  {"type": "PLAYER_MONEY", "player_id": <int>, "money": <int>}
  ```

- `BET_TOTAL`: Player's bet updated
  ```json
  {"type": "BET_TOTAL", "player_id": <int>, "amount": <int>}
  ```

- `MAIN_POT`: Pot amount changed
  ```json
  {"type": "MAIN_POT", "amount": <int>}
  ```

- `SIDE_POTS`: Side pots updated
  ```json
  {"type": "SIDE_POTS", "pots": [{...}]}
  ```

- `TABLE_CARDS`: Community cards revealed
  ```json
  {"type": "TABLE_CARDS", "cards": [<int>, ...]}
  ```

- `PLAYER_CARDS`: Hole cards revealed (private or showdown)
  ```json
  {"type": "PLAYER_CARDS", "player_id": <int>, "cards": [<int>, <int>]}
  ```

- `AVAILABLE_ACTIONS`: Actions available to player
  ```json
  {"type": "AVAILABLE_ACTIONS", "player_id": <int>, "actions": [{...}]}
  ```

- `BUTTON_SEAT`: Dealer button position
  ```json
  {"type": "BUTTON_SEAT", "seat": <int>}
  ```

### Notification Types

**Event Notifications** (for UI/logging):

- `PLAYER_FOLD`: Player folded
  ```json
  {"type": "PLAYER_FOLD", "player_id": <int>}
  ```

- `PLAYER_CHECK`: Player checked
  ```json
  {"type": "PLAYER_CHECK", "player_id": <int>}
  ```

- `PLAYER_CALL`: Player called
  ```json
  {"type": "PLAYER_CALL", "player_id": <int>, "amount": <int>}
  ```

- `PLAYER_BET`: Player bet
  ```json
  {"type": "PLAYER_BET", "player_id": <int>, "amount": <int>}
  ```

- `PLAYER_RAISE`: Player raised
  ```json
  {"type": "PLAYER_RAISE", "player_id": <int>, "amount": <int>}
  ```

- `PLAYER_ALLIN`: Player went all-in
  ```json
  {"type": "PLAYER_ALLIN", "player_id": <int>, "amount": <int>}
  ```

- `FLOP_DEALT`: Flop cards dealt
  ```json
  {"type": "FLOP_DEALT"}
  ```

- `TURN_DEALT`: Turn card dealt
  ```json
  {"type": "TURN_DEALT"}
  ```

- `RIVER_DEALT`: River card dealt
  ```json
  {"type": "RIVER_DEALT"}
  ```

- `SHOWDOWN`: Showdown phase
  ```json
  {"type": "SHOWDOWN"}
  ```

- `HAND_ENDED`: Hand completed
  ```json
  {"type": "HAND_ENDED", "winner_id": <int>, "amount": <int>}
  ```

- `PLAYER_JOINED`: Player joined table
  ```json
  {"type": "PLAYER_JOINED", "player_id": <int>, "name": "<string>", "seat": <int>}
  ```

- `PLAYER_LEFT`: Player left table
  ```json
  {"type": "PLAYER_LEFT", "player_id": <int>, "seat": <int>}
  ```

**Client Rule**: Apply updates strictly by increasing `seq`. Drop duplicates or out-of-order updates. If a gap is detected, request resync.

---

## Player Actions

### ACTION_REQUEST (ID: 450)

**Direction**: C2S

**Payload** (MessagePack):
```json
{
  "game_id": <int>,
  "action": {
    "type": "<string>",    // "fold","check","call","bet","raise","all_in"
    "amount": <int>        // Required for bet/raise, optional for others
  },
  "client_seq": <u32>      // Optional correlation ID
}
```

**Action Types:**
- `fold`: Fold hand
- `check`: Check (no bet)
- `call`: Call current bet
- `bet`: Make initial bet
- `raise`: Raise current bet
- `all_in`: Go all-in

### ACTION_RESULT (ID: 451)

**Direction**: S2C

**Payload** (MessagePack):
```json
{
  "result": <int>,
  "client_seq": <u32>,     // Echoed from request
  "reason": "<string>"     // Present on failure
}
```

**Result Codes:**
- `0`: OK - Action accepted (state changes follow in UPDATE_BUNDLE)
- `400`: BAD_ACTION - Invalid action type
- `403`: NOT_YOUR_TURN - Not the active player
- `409`: STATE_CHANGED - Game state changed since action was sent
- `409`: NOT_ALLOWED - Action not legal in current state
- `422`: INVALID_AMOUNT - Invalid bet/raise amount
- `429`: RATE_LIMIT - Too many requests
- `500`: SERVER_ERROR

**Note**: Successful actions (result=0) are immediately followed by one or more UPDATE_BUNDLE packets containing the actual state changes.

---

## Resynchronization

If a client detects missing sequence numbers or needs to recover state:

### RESYNC_REQUEST (ID: 470)

**Direction**: C2S

**Payload** (MessagePack):
```json
{
  "game_id": <int>
}
```

### RESYNC_RESPONSE (ID: 471)

**Direction**: S2C

**Payload** (MessagePack):
```json
{
  "result": <int>,
  "game_state": {
    // Full GAME_STATE structure
  },
  "seq": <u32>    // Current sequence number
}
```

After receiving resync, client should:
1. Discard any buffered updates with seq <= returned seq
2. Apply the full game_state
3. Resume processing updates with seq > returned seq

---

## Error Handling

### Validation
- Server MUST validate all C2S payloads (types, ranges, turn, balance, state)
- Invalid packets → send ERROR (900) or appropriate error response
- Malformed packets → close connection

### Rate Limiting
- Per-connection and per-IP limits
- Exceeded → ERROR (900) with code=429 or close connection

### Slow Clients
- If send buffer backs up (>5MB or >30s delay) → close connection
- Prevents one slow client from blocking server

### Disconnects
- Player disconnect during hand:
  - If player's turn → auto-fold after timer expires
  - If player not to act → marked as disconnected, auto-folds when turn comes
- Disconnect during betting → player sitting out next hand

### Timeouts
- **Handshake**: 5s to complete
- **Login**: 30s after handshake
- **Idle**: 60s of no application messages
- **Action**: Per-table configuration (typically 20-30s)

---

## Example Flows

### Complete Game Flow

```
1. TCP Connect
   └→ Client sends handshake (v1)
   └→ Server responds HANDSHAKE_OK

2. Authentication
   └→ C: LOGIN_REQUEST {user: "player1", password: "***"}
   └→ S: LOGIN_RESPONSE (101 OK) {user_id: 123, balance: 10000, ...}

3. Browse Tables
   └→ C: GET_TABLES (500)
   └→ S: TABLES_RESPONSE {tables: [{id:1, name:"Table1",...}, ...]}

4. Join Table
   └→ C: JOIN_TABLE_REQUEST {table_id: 1}
   └→ S: JOIN_TABLE_RESPONSE (401 OK) {game_state: {...}}

5. Game Play
   └→ S: UPDATE_BUNDLE {seq:1, notifications:[HAND_STARTED], updates:[...]}
   └→ S: UPDATE_BUNDLE {seq:2, notifications:[], updates:[ACTIVE_PLAYER:123,...]}
   └→ C: ACTION_REQUEST {action:{type:"bet",amount:100}}
   └→ S: ACTION_RESULT {result:0}
   └→ S: UPDATE_BUNDLE {seq:3, notifications:[PLAYER_BET], updates:[...]}
   └→ S: UPDATE_BUNDLE {seq:4, notifications:[FLOP_DEALT], updates:[TABLE_CARDS:[...]]}
   ... (continue play) ...
   └→ S: UPDATE_BUNDLE {seq:N, notifications:[HAND_ENDED], updates:[...]}

6. Leave Table
   └→ C: LEAVE_TABLE_REQUEST {table_id: 1}
   └→ S: LEAVE_TABLE_RESPONSE (701 OK)

7. Disconnect
   └→ Close TCP connection
```

### Action Processing Example

```
Player wants to raise to 300:

C→S: ACTION_REQUEST (450)
{
  "game_id": 1,
  "action": {"type": "raise", "amount": 300},
  "client_seq": 42
}

S→C: ACTION_RESULT (451)
{
  "result": 0,
  "client_seq": 42
}

S→C: UPDATE_BUNDLE (460)
{
  "seq": 118,
  "notifications": [
    {"type": "PLAYER_RAISE", "player_id": 7, "amount": 300}
  ],
  "updates": [
    {"type": "BET_TOTAL", "player_id": 7, "amount": 300},
    {"type": "PLAYER_MONEY", "player_id": 7, "money": 9700},
    {"type": "MAIN_POT", "amount": 1000},
    {"type": "ACTIVE_PLAYER", "player_id": 12},
    {"type": "PLAYER_TIMER", "player_id": 12, "timer": 1730550000000},
    {"type": "AVAILABLE_ACTIONS", "player_id": 12, "actions": [...]}
  ]
}
```

### Error Handling Example

```
Invalid action (not player's turn):

C→S: ACTION_REQUEST (450)
{
  "game_id": 1,
  "action": {"type": "bet", "amount": 100}
}

S→C: ACTION_RESULT (451)
{
  "result": 403,
  "reason": "Not your turn"
}

(No UPDATE_BUNDLE follows)
```

---

## Wire Format Examples

### Handshake

**Client Request (v1):**
```
00 02 00 01
│  │  └──┴── protocol_version = 1
└──┴──────── length = 2
```

**Server Response (OK):**
```
00 01 00
│  │  └──── code = 0 (OK)
└──┴──────── length = 1
```

### Login Request

**Packet (hex):**
```
00 1D 01 00 64  [MessagePack payload...]
│  │  │  └──┴── packet_type = 100 (LOGIN)
│  │  └──────── protocol_ver = 1
└──┴──────────── packet_len = 29
```

**MessagePack Payload (hex):**
```
82 A4 75 73 65 72 A5 61 6C 69 63 65 A8 70 61 73 73 77 6F 72 64 A4 31 32 33 34
│  │  └─────────┴───────────────────── "user": "alice"
│  └────────────────────────────────── map with 2 items
└───────────────────────────────────── "password": "1234"
```

**Decoded:**
```json
{"user": "alice", "password": "1234"}
```

---

## Implementation Notes

### Game Engine Architecture

The game logic is implemented in a modular fashion in the `pokergame` library:

**Core Modules:**

1. **Game State Management** (`game_engine.h/c`)
   - `GameState` structure: Central game state
   - State initialization and cleanup
   - Hand lifecycle management

2. **Player Management**
   - Add/remove players
   - Track player states (active, folded, all-in, etc.)
   - Manage player chips and bets

3. **Betting Logic**
   - Post blinds
   - Validate actions (fold, check, call, bet, raise, all-in)
   - Process player actions
   - Manage betting rounds
   - Calculate pots and side pots

4. **Hand Progression**
   - Deal hole cards
   - Deal community cards (flop, turn, river)
   - Advance betting rounds
   - Determine winners
   - Showdown logic

5. **Action Validation**
   - Check turn order
   - Validate bet amounts (min/max)
   - Ensure sufficient chips
   - Generate available actions

### State Synchronization

- **Sequence Numbers**: Every state change increments `seq`
- **Atomic Updates**: All related changes bundled in single UPDATE_BUNDLE
- **Client Model**: Client maintains local game state, applies updates by seq
- **Missed Updates**: Client detects gaps, requests RESYNC

### Best Practices

1. **Always validate actions server-side** - Never trust client
2. **Use sequence numbers** - Ensure ordered update delivery
3. **Bundle related updates** - Atomic state transitions
4. **Handle disconnects gracefully** - Auto-fold, allow reconnect
5. **Rate limit actions** - Prevent spam/DoS
6. **Log all actions** - For audit and debugging
7. **Use TLS in production** - Encrypt sensitive data
8. **Implement proper timeouts** - Prevent hung connections

### Testing Considerations

1. **Unit Tests**: Test individual game logic functions
2. **Integration Tests**: Test packet encoding/decoding
3. **Load Tests**: Simulate multiple concurrent games
4. **Chaos Tests**: Random disconnects, delayed messages
5. **Security Tests**: Malformed packets, invalid actions

---

## Compatibility and Evolution

### Adding New Features

**Safe (backward compatible):**
- Add new optional fields to existing structures
- Add new packet types
- Add new update/notification types
- Add new error codes

**Breaking (requires version bump):**
- Change existing field types
- Remove fields
- Change packet formats
- Change existing error codes

### Version Migration

When introducing breaking changes:
1. Increment protocol version
2. Update handshake to support multiple versions
3. Maintain server support for old versions during transition
4. Deprecated old version after grace period

---

## Security Considerations

1. **Authentication**: Use secure password hashing (bcrypt, argon2)
2. **Transport**: Always use TLS in production
3. **Input Validation**: Validate all inputs server-side
4. **Rate Limiting**: Prevent abuse and DoS
5. **Session Management**: Secure session tokens, timeout inactive sessions
6. **Audit Logging**: Log all actions for forensics
7. **Balance Validation**: Prevent chip manipulation
8. **Action Validation**: Ensure players can't act out of turn or with invalid amounts

---

## Appendix: Data Structures

### C Structures

```c
// Protocol header
typedef struct __attribute__((packed)) {
    uint16_t packet_len;
    uint8_t protocol_ver;
    uint16_t packet_type;
} Header;

// Game state
typedef struct {
    int game_id;
    uint32_t hand_id;
    uint32_t seq;
    BettingRound betting_round;
    int dealer_seat;
    int active_seat;
    GamePlayer players[MAX_PLAYERS];
    Card *community_cards[5];
    Pot main_pot;
    // ... (see game_engine.h for complete definition)
} GameState;

// Player action
typedef struct {
    ActionType type;
    int amount;
} Action;

// Available action
typedef struct {
    ActionType type;
    int min_amount;
    int max_amount;
    int increment;
} AvailableAction;
```

---

## Version History

- **v1.0** (Current): Initial protocol specification
  - Basic authentication
  - Table management
  - Game join/leave
  - In-game actions with UPDATE_BUNDLE
  - Resynchronization support

---

## Contact and Support

For questions, issues, or contributions:
- Repository: https://github.com/datamonsterr/Cardio
- Documentation: /PROTOCOL.md

---

**END OF PROTOCOL SPECIFICATION**
