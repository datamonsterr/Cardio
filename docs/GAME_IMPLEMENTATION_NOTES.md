# Game Logic Implementation Notes

## Overview
This document describes the implementation of multiplayer Texas Hold'em poker game logic in the Cardio server.

## Date
January 9, 2026

## Implementation Summary

### 1. Core Architecture

#### Table Structure Extension
- Extended `Table` struct in [game.h](../server/include/game.h) to include:
  - `GameState* game_state` - Pointer to game engine state
  - `conn_data_t* connections[MAX_PLAYERS]` - Array of connected players
  - `int seat_to_conn_idx[MAX_PLAYERS]` - Mapping from seat to connection index

#### Connection Tracking
- Added `seat` field to `conn_data_t` in [server.h](../server/include/server.h)
  - Tracks which seat a player occupies at a table (-1 if not seated)
  - Enables efficient player lookup and broadcast

### 2. Protocol Implementation

#### New Packet Types
- `PACKET_ACTION_REQUEST` (450) - Player action (fold, check, call, bet, raise)
- `PACKET_ACTION_RESULT` (451) - Server response to action
- `PACKET_UPDATE_GAMESTATE` (600) - Game state broadcast
- `PACKET_UPDATE_BUNDLE` (460) - Incremental updates (for future use)
- `PACKET_RESYNC_REQUEST` (470) - Client requests full state
- `PACKET_RESYNC_RESPONSE` (471) - Full state resync

#### New Protocol Structures
Added to [protocol.h](../server/include/protocol.h):
```c
typedef struct {
    int game_id;
    char action_type[16];  // "fold", "check", "call", "bet", "raise", "all_in"
    int amount;
    uint32_t client_seq;
} ActionRequest;

typedef struct {
    int result;            // 0=OK, 400=BAD_ACTION, 403=NOT_YOUR_TURN, etc.
    uint32_t client_seq;
    char reason[128];
} ActionResult;
```

#### Encode/Decode Functions
Created [protocol_game.c](../server/src/protocol_game.c) with:
- `decode_action_request()` - Parse player action from MessagePack
- `encode_action_result()` - Format action response
- `encode_game_state()` - Serialize full game state (shows cards only to owning player)
- `encode_update_bundle()` - Incremental updates (stub for now)

#### Card Encoding
Cards represented as integers following the protocol:
- `-1` = Hidden/unknown card
- Valid cards: `suit * 13 + (rank - 2)`
  - Suits: 1=Spades, 2=Hearts, 3=Diamonds, 4=Clubs
  - Ranks: 2-14 (where 14=Ace)

### 3. Game State Management

#### Table Creation
Modified [game_room.c](../server/src/game_room.c) `add_table()`:
- Initializes `GameState` for each new table
- Sets up blinds (small_blind = min_bet/2, big_blind = min_bet)
- Initializes connection tracking arrays

#### Joining Tables
Enhanced `join_table()`:
1. Finds empty seat in game state
2. Adds player to GameState with default buy-in (50 big blinds)
3. Tracks connection in table's connections array
4. Maps seat number to connection index

Enhanced `handle_join_table_request()`:
1. Calls `join_table()` to add player to table
2. Checks if game should start (2+ players, no hand in progress)
3. If ready, calls `game_start_hand()` directly to start the hand
4. Encodes and sends join response with current game state to joining player
5. If game just started, broadcasts `PACKET_UPDATE_GAMESTATE` to OTHER players
   - Skips the joining player (they already received state in join response)
   - Prevents race condition where joining player receives duplicate/out-of-order messages

#### Leaving Tables
Updated `leave_table()`:
1. Removes player from GameState
2. Cleans up connection tracking
3. Destroys table if last player leaves

#### Broadcasting
Implemented `broadcast_to_table()`:
- Sends packets to all connected players at a table
- Handles failed sends gracefully
- Used for game state updates after actions

### 4. Game Flow

#### Auto-Start Logic
Function `start_game_if_ready()` in [game_room.c](../server/src/game_room.c):
- Called after actions complete a hand (betting round = COMPLETE)
- Checks for minimum 2 active players
- Prevents restart if hand already in progress
- Calls `game_start_hand()` from game engine
- Broadcasts game state to all players at table

**Note:** Join handler (`handle_join_table_request`) manages game start separately to avoid 
message ordering issues. It starts the game, sends join response, then broadcasts to other players.

#### Action Processing
Implemented `handle_action_request()` in [handler.c](../server/src/handler.c):

**Validation Steps:**
1. Check player is logged in and at a table
2. Verify table exists
3. Decode action request
4. Confirm it's player's turn
5. Convert action type string to enum
6. Validate action with `game_validate_action()`
7. Process action with `game_process_action()`

**Response Flow:**
1. Send `ACTION_RESULT` to requesting player
   - result=0 on success
   - result=403 if not player's turn
   - result=409 if action invalid
   - result=500 on processing error
2. Broadcast updated `PACKET_UPDATE_GAMESTATE` to all players at table
   - Each player receives personalized game state (sees only own cards)

### 5. Main Server Loop Integration

Updated [main.c](../server/src/main.c):
- Added case for `PACKET_ACTION_REQUEST` in switch statement
- Calls `handle_action_request()` with table_list
- Handles disconnection cleanup (calls `leave_table()` if player at table)

### 6. Unit Tests

Added tests in [unit_test.c](../server/test/unit_test.c):
- `test_decode_action_request()` - Verifies MessagePack decoding
- `test_encode_action_result()` - Validates response encoding

All tests passing ✓

### 7. Build Configuration

Updated [CMakeLists.txt](../server/CMakeLists.txt):
- Reordered libraries to fix linker dependencies
- pokergame before card (pokergame depends on card)

## Recent Fixes (January 10, 2026)

### Game Start Broadcasting Issue

**Problem 1:** When second player joined, game would start and broadcast to all players including 
the joining player. This caused the joining player to receive:
1. `PACKET_UPDATE_GAMESTATE` (broadcast)
2. `PACKET_JOIN_TABLE` (join response)

Test clients reading with blocking `recv()` would get the broadcast first and fail to recognize 
it as a join response.

**Problem 2:** Used `game_count_active_players()` to check if game should start, but this 
only counts players in ACTIVE/ALL_IN state. Newly joined players are in WAITING state until 
`game_start_hand()` is called, so the count was always less than 2 and game never started.

**Solution:** Modified `handle_join_table_request()` to:
1. Check `gs->num_players >= 2` instead of `game_count_active_players()` (counts all non-empty players including WAITING)
2. Start game inline by calling `game_start_hand()` directly instead of using `start_game_if_ready()`
3. Send join response with updated game state to joining player first
4. Broadcast `PACKET_UPDATE_GAMESTATE` to OTHER players only (skip the joining player)
5. Added debug logging to trace player states, dealer, blinds, and active_seat after game start

This ensures:
- Game starts when 2+ players are present (regardless of player state)
- Joining player receives game state in their join response
- Other players receive update broadcasts
- No message ordering issues for blocking clients

**Note:** The game engine's `game_start_hand()` automatically converts WAITING players to ACTIVE 
state before dealing cards and setting positions.

**Status: FIXED and VERIFIED** (January 10, 2026)

Game start logic is now working correctly:
- Game starts when 2+ players join (verified in server logs)
- `active_seat` is set correctly to first player after big blind
- Game state is encoded with correct `active_seat` value
- Join responses contain full game state with active_seat
- Broadcasts sent to other players with updated game state

**Server logs confirm:**
```
[INFO] Game started: hand_id=1 dealer_seat=0 active_seat=1 betting_round=0
[DEBUG] [encode_game_state] Encoding active_seat=1 for viewer=165
[DEBUG] [encode_game_state] Encoding active_seat=1 for viewer=164
```

**Note:** E2E test issue in SCENARIO 4 is a test logic problem, not a server issue. After game starts, 
there are no more broadcasts until a player takes an action. Test should use `active_seat` from join 
responses rather than waiting for new broadcasts.

## Current Limitations & TODOs

### 1. Incremental Updates
Currently sends full game state after each action. Should implement:
- Notification system (PLAYER_FOLD, PLAYER_RAISE, etc.)
- Delta updates (only changed fields)
- Sequence numbering for update tracking

### 2. Bot Replacement
No bot implementation for disconnected players yet. Should:
- Detect player disconnect during hand
- Replace with bot that auto-folds/checks
- Remove bot when hand completes

### 3. Timer Management
No action timer implementation. Should:
- Set `timer_deadline` when player's turn starts
- Check timer expiration in event loop
- Auto-fold on timeout

### 4. Hand History
No hand history or result tracking. Should:
- Log hand outcomes
- Track winnings/losses
- Update player balances in database

### 5. Reconnection
No reconnection support. Should:
- Track disconnected players
- Allow reconnect within timeout
- Send resync on reconnect

### 6. Multi-Hand Flow
Game starts one hand but doesn't automatically continue. Should:
- Detect hand completion
- Start new hand after delay
- Handle player sit-out/stand-up

### 7. Side Pots
Game engine has side pot support but not tested in multiplayer context.

### 8. Client Implementation
Test clients (client.c, e2e_multiplayer_test.c) need updates:
- Add action sending logic
- Parse game state updates
- Display game state to user
- Handle available actions

## Testing Strategy

### Unit Tests
- Protocol encoding/decoding ✓
- Game state serialization (TODO)
- Action validation (uses engine tests)

### Integration Tests
Current: `e2e_multiplayer_test.c`
- Needs completion for:
  - Multiple players joining
  - Full hand gameplay
  - Betting rounds
  - Showdown

### Manual Testing
Using `client.c`:
- Need to add menu options for:
  - Fold, Check, Call, Bet, Raise
  - Display current game state
  - Show available actions

## Game Engine Integration

Leverages existing [game_engine.h](../server/lib/pokergame/include/game_engine.h):
- ✓ `game_state_create()` - Initialize table
- ✓ `game_add_player()` - Seat players
- ✓ `game_remove_player()` - Unseat players
- ✓ `game_start_hand()` - Begin new hand
- ✓ `game_validate_action()` - Check action legality
- ✓ `game_process_action()` - Apply action to state
- ✓ `game_get_available_actions()` - Get valid actions for player
- ⚠ `game_advance_betting_round()` - Called automatically by engine
- ⚠ `game_determine_winner()` - Called at showdown
- TODO: Hook game end events to continue next hand

## Protocol Conformance

Implementation follows [PROTOCOL.md](PROTOCOL.md) specification:
- ✓ MESSAGE framing (5-byte header + MessagePack payload)
- ✓ ACTION_REQUEST format
- ✓ ACTION_RESULT codes
- ✓ GAME_STATE structure
- ✓ Card encoding scheme
- ⚠ UPDATE_BUNDLE (stub only, sends full state for now)
- ✗ RESYNC_REQUEST/RESPONSE (not implemented)
- ✗ Sequence number tracking (not implemented)

## Database Integration

Current state:
- Player join uses default buy-in from balance (not validated)
- No balance updates on hand completion
- No hand history storage

TODO:
- Validate buy-in against player balance
- Deduct buy-in on join
- Update balance on leave
- Store hand results

## Error Handling

Implemented:
- Invalid packet validation
- Turn validation
- Action validation
- Send failures (logged, continue serving other players)

Not implemented:
- Rate limiting
- Timeout enforcement
- Malformed packet handling (partial)

## Performance Considerations

Current approach:
- Full game state broadcast after each action
- O(n) broadcast to all players
- No batching of updates

Optimizations needed:
- Incremental updates
- Update batching
- Diff-based broadcasts

## Security Considerations

✓ Players see only their own hole cards (in encode_game_state)
✓ Action validation prevents cheating (turn order, valid actions)
✗ No balance verification against database
✗ No rate limiting on actions
✗ No timeout to prevent game stalling

## File Structure

New files:
- `server/src/protocol_game.c` - Game protocol encoding/decoding

Modified files:
- `server/include/protocol.h` - Added action packet structures
- `server/include/game.h` - Extended Table structure
- `server/include/server.h` - Added seat tracking, function declarations
- `server/src/game_room.c` - Game state initialization, broadcasting
- `server/src/handler.c` - Action handler, join/leave enhancements
- `server/src/server.c` - Seat initialization
- `server/src/main.c` - Action packet routing
- `server/test/unit_test.c` - Action protocol tests
- `server/test/e2e_multiplayer_test.c` - Fixed naming conflict

## Next Steps

Immediate priorities:
1. Complete e2e_multiplayer_test.c with full gameplay
2. Update client.c with action menu and game display
3. Implement hand continuation logic
4. Add basic bot replacement for disconnects
5. Implement timer system

Future enhancements:
1. Incremental update system
2. Reconnection support
3. Database balance integration
4. Hand history storage
5. Spectator mode
6. Tournament support

## Notes

- Game engine (lib/pokergame) is well-tested and handles all poker logic
- Server acts as thin wrapper routing actions to game engine
- Protocol design allows for future optimizations without breaking changes
- Architecture supports multiple concurrent tables
- epoll-based event loop scales well for many connections
