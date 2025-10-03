# Poker Game Logic Implementation Documentation

## Overview

This document describes the implementation of the poker game logic in the Cardio server. The implementation follows a modular, best-practice architecture that separates concerns and provides clear interfaces between components.

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       Client Layer                           │
│                    (Mobile/Web App)                          │
└────────────────────────┬────────────────────────────────────┘
                         │ TCP/TLS + Protocol
                         │
┌────────────────────────▼────────────────────────────────────┐
│                    Server Layer                              │
│  ┌────────────────────────────────────────────────────┐     │
│  │           Connection Handler (main.c)               │     │
│  │  - Epoll event loop                                 │     │
│  │  - Connection management                            │     │
│  │  - Packet routing                                   │     │
│  └──────────────┬─────────────────────────────────────┘     │
│                 │                                            │
│  ┌──────────────▼─────────────────────────────────────┐     │
│  │         Protocol Layer (protocol.c/h)               │     │
│  │  - Packet encoding/decoding                         │     │
│  │  - MessagePack serialization                        │     │
│  │  - Header parsing                                   │     │
│  └──────────────┬─────────────────────────────────────┘     │
│                 │                                            │
│  ┌──────────────▼─────────────────────────────────────┐     │
│  │          Handler Layer (handler.c)                  │     │
│  │  - Request dispatch                                 │     │
│  │  - Business logic coordination                      │     │
│  │  - Response generation                              │     │
│  └──────────────┬─────────────────────────────────────┘     │
│                 │                                            │
│  ┌──────────────▼─────────────────────────────────────┐     │
│  │        Game Engine (pokergame library)              │     │
│  │                                                      │     │
│  │  ┌─────────────────────────────────────────┐       │     │
│  │  │    Game State Management                 │       │     │
│  │  │  - GameState structure                   │       │     │
│  │  │  - State initialization                  │       │     │
│  │  │  - Hand lifecycle                        │       │     │
│  │  └─────────────────────────────────────────┘       │     │
│  │                                                      │     │
│  │  ┌─────────────────────────────────────────┐       │     │
│  │  │    Player Management                     │       │     │
│  │  │  - Add/remove players                    │       │     │
│  │  │  - Player state tracking                 │       │     │
│  │  │  - Chip management                       │       │     │
│  │  └─────────────────────────────────────────┘       │     │
│  │                                                      │     │
│  │  ┌─────────────────────────────────────────┐       │     │
│  │  │    Betting Logic                         │       │     │
│  │  │  - Action validation                     │       │     │
│  │  │  - Action processing                     │       │     │
│  │  │  - Pot management                        │       │     │
│  │  │  - Betting round control                 │       │     │
│  │  └─────────────────────────────────────────┘       │     │
│  │                                                      │     │
│  │  ┌─────────────────────────────────────────┐       │     │
│  │  │    Hand Progression                      │       │     │
│  │  │  - Card dealing                          │       │     │
│  │  │  - Round advancement                     │       │     │
│  │  │  - Winner determination                  │       │     │
│  │  │  - Showdown logic                        │       │     │
│  │  └─────────────────────────────────────────┘       │     │
│  └──────────────┬─────────────────────────────────────┘     │
│                 │                                            │
│  ┌──────────────▼─────────────────────────────────────┐     │
│  │     Supporting Libraries                            │     │
│  │  - card: Card and deck management                   │     │
│  │  - db: Database operations                          │     │
│  │  - mpack: MessagePack encoding                      │     │
│  │  - logger: Logging utilities                        │     │
│  └─────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

## Module Design

### 1. Game Engine Module (`game_engine.h/c`)

The game engine is the core of the poker game logic, implementing all game rules and state management.

#### Key Data Structures

**GameState**
```c
typedef struct {
    // Identification
    int game_id;
    uint32_t hand_id;
    uint32_t seq;
    
    // Configuration
    int max_players;
    int small_blind;
    int big_blind;
    
    // Current state
    BettingRound betting_round;
    int dealer_seat;
    int active_seat;
    
    // Players and cards
    GamePlayer players[MAX_PLAYERS];
    Card *community_cards[5];
    
    // Pots
    Pot main_pot;
    Pot side_pots[MAX_PLAYERS];
    
    // Betting
    int current_bet;
    int min_raise;
    
    Deck *deck;
    bool hand_in_progress;
} GameState;
```

**GamePlayer**
```c
typedef struct {
    int player_id;
    char name[32];
    int seat;
    PlayerState state;
    int money;
    int bet;
    int total_bet;
    Card *hole_cards[2];
    bool is_dealer;
    bool is_small_blind;
    bool is_big_blind;
    uint64_t timer_deadline;
} GamePlayer;
```

#### Design Principles

1. **Immutable State**: State changes only through defined functions
2. **Validation First**: Always validate before modifying state
3. **Atomic Updates**: Related changes happen together
4. **Clear Ownership**: GameState owns all game data
5. **No Side Effects**: Functions don't modify global state

### 2. State Management

#### State Lifecycle

```
EMPTY → WAITING → ACTIVE → [FOLDED | ALL_IN] → WAITING
  ↓                                                 ↑
SITTING_OUT ────────────────────────────────────────┘
```

#### Functions

**Creation and Destruction**
```c
GameState* game_state_create(int game_id, int max_players, 
                             int small_blind, int big_blind);
void game_state_destroy(GameState *state);
void game_state_reset_for_new_hand(GameState *state);
```

**Player Management**
```c
int game_add_player(GameState *state, int player_id, 
                    const char *name, int seat, int buy_in);
int game_remove_player(GameState *state, int seat);
GamePlayer* game_get_player_by_id(GameState *state, int player_id);
GamePlayer* game_get_player_by_seat(GameState *state, int seat);
```

### 3. Action Processing Pipeline

```
Client Action Request
         ↓
    Validation
    - Check turn
    - Check amounts
    - Check legality
         ↓
    Processing
    - Update player state
    - Modify chips/bets
    - Update game state
         ↓
    Side Effects
    - Check round complete
    - Advance round if needed
    - Move to next player
         ↓
    Update Generation
    - Create notifications
    - Create state updates
    - Bundle atomically
         ↓
    Send to Clients
```

#### Action Validation

```c
ActionValidation game_validate_action(GameState *state, 
                                     int player_id, 
                                     Action *action);

typedef struct {
    bool is_valid;
    const char *error_message;
    int adjusted_amount;
} ActionValidation;
```

**Validation Rules:**
- **Fold**: Always valid
- **Check**: Only if no bet to call
- **Call**: Only if there's a bet; amount = bet to call
- **Bet**: Only if no current bet; amount >= big blind
- **Raise**: Only if there's a bet; amount >= current_bet + min_raise
- **All-in**: Always valid if player has chips

#### Action Processing

```c
int game_process_action(GameState *state, int player_id, Action *action);
```

**Processing Steps:**
1. Validate action
2. Update player state (chips, bet, state)
3. Update game state (current_bet, min_raise, pot)
4. Increment sequence number
5. Check if round complete
6. Advance round or move to next player

### 4. Hand Lifecycle

#### Hand Phases

```
HAND_START
    ↓
Post Blinds
    ↓
Deal Hole Cards
    ↓
PREFLOP Betting
    ↓
Deal Flop (3 cards)
    ↓
FLOP Betting
    ↓
Deal Turn (1 card)
    ↓
TURN Betting
    ↓
Deal River (1 card)
    ↓
RIVER Betting
    ↓
SHOWDOWN
    ↓
Determine Winner
    ↓
Distribute Pot
    ↓
HAND_END
```

#### Implementation

```c
int game_start_hand(GameState *state);
int game_deal_hole_cards(GameState *state);
int game_post_blinds(GameState *state);
int game_advance_betting_round(GameState *state);
int game_showdown(GameState *state);
int game_end_hand(GameState *state);
```

**Key Features:**
- Automatic dealer button rotation
- Blind position calculation
- Deck shuffling and dealing
- Burn cards (before flop, turn, river)
- Community card management
- Winner determination

### 5. Betting Logic

#### Betting Round Completion

A betting round is complete when:
1. All active players have acted AND
2. All active players have matched the current bet OR are all-in OR
3. Only one player remains (all others folded)

```c
bool game_is_betting_round_complete(GameState *state);
```

#### Pot Management

**Main Pot Collection:**
```c
void game_collect_bets_to_pot(GameState *state);
```
- Called at end of each betting round
- Moves all bets into main pot
- Resets player bet amounts

**Side Pots:**
```c
void game_calculate_side_pots(GameState *state);
```
- Handles all-in scenarios
- Creates side pots for players with limited stakes
- Tracks eligible players for each pot

**Distribution:**
```c
int game_distribute_pot(GameState *state, int winning_seat);
```
- Awards pot to winner
- Updates winner's stack
- Clears pot amounts

### 6. Update Bundle Generation

When game state changes, server generates UPDATE_BUNDLE messages containing:

**Notifications** (for UI/animation):
- PLAYER_BET, PLAYER_RAISE, PLAYER_FOLD, etc.
- User-facing events
- Can be ignored by simple clients

**Updates** (for state sync):
- BET_TOTAL, MAIN_POT, ACTIVE_PLAYER, etc.
- Required for maintaining correct game state
- Must be applied by all clients

**Example Generation:**
```c
// After player raises to 300
UpdateBundle bundle = {
    .seq = state->seq,
    .notifications = [
        {.type = "PLAYER_RAISE", .player_id = 7, .amount = 300}
    ],
    .updates = [
        {.type = "BET_TOTAL", .player_id = 7, .amount = 300},
        {.type = "PLAYER_MONEY", .player_id = 7, .money = 9700},
        {.type = "MAIN_POT", .amount = 1000},
        {.type = "ACTIVE_PLAYER", .player_id = 12},
        {.type = "PLAYER_TIMER", .player_id = 12, .timer = deadline}
    ]
};
```

## Best Practices

### 1. Modular Design

**Separation of Concerns:**
- Game logic separate from networking
- Protocol encoding separate from game rules
- Database separate from game state

**Benefits:**
- Easier testing
- Easier maintenance
- Clear interfaces
- Reusable components

### 2. State Management

**Single Source of Truth:**
- GameState is authoritative
- All game data in one structure
- Clear ownership

**Immutability:**
- State changes through functions only
- No direct field modification from outside
- Validates all changes

### 3. Error Handling

**Defensive Programming:**
```c
if (!state || !action) return -1;
if (player->seat != state->active_seat) {
    return ERROR_NOT_YOUR_TURN;
}
```

**Clear Error Codes:**
- Negative for system errors
- Positive for business logic errors
- Zero for success

### 4. Memory Management

**Resource Ownership:**
- GameState owns deck
- GameState owns all cards
- Clear allocation/deallocation

**Cleanup:**
```c
void game_state_destroy(GameState *state) {
    if (!state) return;
    if (state->deck) {
        deck_destroy(state->deck);
    }
    free(state);
}
```

### 5. Testing Strategy

**Unit Tests:**
- Test individual functions
- Mock dependencies
- Test edge cases

**Integration Tests:**
- Test full hand lifecycle
- Test multiple players
- Test error conditions

**Scenarios to Test:**
- Normal hand completion
- All players fold
- One player all-in
- Multiple all-ins (side pots)
- Player disconnect
- Invalid actions
- Sequence number gaps

## Integration with Server

### Connection Handler Integration

```c
// In handler.c
void handle_action_request(conn_data_t *conn_data, 
                          char *data, size_t data_len,
                          TableList *table_list) {
    // 1. Decode packet
    Packet *packet = decode_packet(data, data_len);
    ActionRequest *request = decode_action_request(packet->data);
    
    // 2. Find game state
    GameState *state = get_game_state(conn_data->table_id);
    
    // 3. Process action
    Action action = {
        .type = parse_action_type(request->action_type),
        .amount = request->amount
    };
    
    int result = game_process_action(state, conn_data->user_id, &action);
    
    // 4. Send result
    RawBytes *response = encode_action_result(result);
    send_packet(conn_data->fd, PACKET_ACTION_RESULT, response);
    
    // 5. If successful, broadcast updates
    if (result == 0) {
        UpdateBundle *bundle = generate_update_bundle(state);
        broadcast_to_table(table_id, bundle);
    }
}
```

### Update Broadcasting

```c
void broadcast_to_table(int table_id, UpdateBundle *bundle) {
    // Find all connections at this table
    for each conn at table_id {
        // Encode bundle
        RawBytes *encoded = encode_update_bundle(bundle);
        
        // Send to client
        send_packet(conn->fd, PACKET_UPDATE_BUNDLE, encoded);
        
        free(encoded);
    }
}
```

## Future Enhancements

### Planned Features

1. **Side Pot Calculation**
   - Full implementation for complex all-in scenarios
   - Multiple side pots with different eligible players

2. **Hand Strength Evaluation**
   - Integration with community cards
   - Full 7-card evaluation (2 hole + 5 community)
   - Tie breaking

3. **Tournament Support**
   - Blind level increases
   - Prize pool distribution
   - Elimination tracking

4. **Multi-table Support**
   - Table balancing
   - Table breaking
   - Final table

5. **Advanced Features**
   - Hand history recording
   - Replay functionality
   - Statistics tracking
   - Achievements

### Extensibility Points

**New Game Variants:**
```c
typedef struct {
    GameType type;  // HOLDEM, OMAHA, SEVEN_CARD, etc.
    int cards_per_player;
    int max_community_cards;
    bool use_low_hand;
} GameVariant;
```

**Betting Structures:**
```c
typedef enum {
    LIMIT_FIXED,
    LIMIT_POT,
    LIMIT_NO_LIMIT
} LimitStructure;
```

**Configurable Rules:**
```c
typedef struct {
    int action_timeout;
    int min_players;
    int max_players;
    bool allow_straddle;
    bool allow_run_it_twice;
} GameRules;
```

## Performance Considerations

### Memory Usage

- GameState: ~2-3 KB per table
- Minimal allocations during hand
- Deck reused between hands
- Card objects persistent

### CPU Usage

- O(1) action validation
- O(n) pot distribution (n = players)
- O(n) winner determination
- Efficient state updates

### Scalability

- Stateless protocol (except game state)
- Horizontal scaling possible
- Each table independent
- Lock-free for separate tables

## Security Considerations

### Input Validation

```c
ActionValidation game_validate_action(GameState *state, 
                                     int player_id, 
                                     Action *action) {
    // Validate player exists
    // Validate it's player's turn
    // Validate action is legal
    // Validate amounts are correct
    // Validate player has chips
}
```

### State Protection

- No direct state modification
- All changes through validated functions
- Server is authoritative
- Client cannot manipulate state

### Audit Trail

- Every action logged with:
  - Timestamp
  - Player ID
  - Action type and amount
  - Resulting state
  - Sequence number

## Debugging and Monitoring

### Logging

```c
logger(GAME_LOG, "INFO", 
       "Player %d action %s amount %d at seat %d", 
       player_id, action_type_string(action->type), 
       action->amount, player->seat);
```

### State Inspection

```c
void game_state_dump(GameState *state) {
    printf("Game %d, Hand %d, Seq %u\n", 
           state->game_id, state->hand_id, state->seq);
    printf("Round: %s, Dealer: %d, Active: %d\n",
           betting_round_string(state->betting_round),
           state->dealer_seat, state->active_seat);
    // ... dump all relevant state
}
```

### Metrics

- Hands per hour
- Average pot size
- Action latency
- Player counts
- Error rates

## Conclusion

The poker game logic implementation follows modern software engineering best practices:

- **Modular**: Clear separation of concerns
- **Testable**: Pure functions, clear interfaces
- **Maintainable**: Well-documented, consistent style
- **Scalable**: Efficient algorithms, minimal state
- **Secure**: Input validation, server authority
- **Extensible**: Easy to add new features

The protocol is designed to be:

- **Efficient**: Binary format, minimal overhead
- **Resilient**: Sequence numbers, resync capability
- **Evolvable**: Version negotiation, backward compatibility
- **Complete**: Covers all game scenarios

This architecture provides a solid foundation for building a production-quality poker server.

## References

- [PROTOCOL.md](PROTOCOL.md) - Complete protocol specification
- [game_engine.h](backend/lib/pokergame/include/game_engine.h) - Game engine API
- [protocol.h](backend/include/protocol.h) - Protocol packet definitions
- [README.md](README.md) - Build and setup instructions
