# Poker Game Engine Library

A modular, production-ready poker game engine implementing Texas Hold'em poker rules.

## Overview

This library provides a complete game engine for Texas Hold'em poker with:
- State management for 2-9 player games
- Full betting logic (fold, check, call, bet, raise, all-in)
- Automatic blind posting and dealer button rotation
- Hand lifecycle management (preflop, flop, turn, river, showdown)
- Action validation and processing
- Pot management (main pot and side pots)
- Winner determination

## Quick Start

### Basic Usage

```c
#include "game_engine.h"

// Create a new game
GameState *game = game_state_create(
    1,      // game_id
    6,      // max_players
    10,     // small_blind
    20      // big_blind
);

// Add players
game_add_player(game, 101, "Alice", 0, 1000);  // player_id, name, seat, buy_in
game_add_player(game, 102, "Bob", 1, 1000);
game_add_player(game, 103, "Charlie", 2, 1000);

// Start a hand
game_start_hand(game);

// Process player actions
Action action = {ACTION_CALL, 0};
int result = game_process_action(game, 101, &action);
if (result == 0) {
    // Action successful, game state updated
    printf("Current pot: %d\n", game_get_pot_total(game));
}

// Clean up
game_state_destroy(game);
```

## API Reference

### Game State Management

#### Create/Destroy

```c
GameState* game_state_create(int game_id, int max_players, 
                             int small_blind, int big_blind);
void game_state_destroy(GameState *state);
void game_state_reset_for_new_hand(GameState *state);
```

**Example:**
```c
GameState *game = game_state_create(1, 9, 5, 10);
// ... use game ...
game_state_destroy(game);
```

### Player Management

#### Add/Remove Players

```c
int game_add_player(GameState *state, int player_id, 
                   const char *name, int seat, int buy_in);
int game_remove_player(GameState *state, int seat);
```

**Returns:**
- `0`: Success
- `-1`: Invalid parameters
- `-2`: Seat occupied (add) or empty (remove)
- `-3`: Invalid buy-in

**Example:**
```c
// Add player at seat 0 with 1000 chips
int result = game_add_player(game, 101, "Alice", 0, 1000);
if (result == 0) {
    printf("Player added successfully\n");
}

// Remove player from seat 0
game_remove_player(game, 0);
```

#### Lookup Players

```c
GamePlayer* game_get_player_by_id(GameState *state, int player_id);
GamePlayer* game_get_player_by_seat(GameState *state, int seat);
int game_count_active_players(GameState *state);
```

**Example:**
```c
GamePlayer *player = game_get_player_by_id(game, 101);
if (player) {
    printf("Player %s has %d chips\n", player->name, player->money);
}
```

### Hand Lifecycle

```c
int game_start_hand(GameState *state);
int game_end_hand(GameState *state);
```

**Hand Flow:**
1. `game_start_hand()` - Initializes hand, posts blinds, deals cards
2. Players act in sequence
3. Betting rounds advance automatically when complete
4. `game_end_hand()` - Determines winner, distributes pot

**Example:**
```c
if (game_has_minimum_players(game)) {
    game_start_hand(game);
    // Hand is now in progress
    printf("Hand #%u started\n", game->hand_id);
}
```

### Action Processing

#### Validate Actions

```c
ActionValidation game_validate_action(GameState *state, 
                                     int player_id, 
                                     Action *action);
```

**Returns:**
```c
typedef struct {
    bool is_valid;
    const char *error_message;
    int adjusted_amount;  // Adjusted if needed (e.g., all-in)
} ActionValidation;
```

**Example:**
```c
Action action = {ACTION_RAISE, 100};
ActionValidation validation = game_validate_action(game, 101, &action);
if (validation.is_valid) {
    // Action is legal
} else {
    printf("Invalid action: %s\n", validation.error_message);
}
```

#### Process Actions

```c
int game_process_action(GameState *state, int player_id, Action *action);
```

**Returns:**
- `0`: Success
- `-1`: Invalid parameters
- `-2`: Action validation failed
- `-3`: Player not found

**Example:**
```c
Action fold = {ACTION_FOLD, 0};
if (game_process_action(game, 101, &fold) == 0) {
    printf("Player folded\n");
    // Game state automatically updated
    // Move to next player or advance round
}
```

#### Get Available Actions

```c
void game_get_available_actions(GameState *state, int player_id,
                               AvailableAction *actions, 
                               int *num_actions);
```

**Example:**
```c
AvailableAction actions[10];
int num_actions = 0;
game_get_available_actions(game, 101, actions, &num_actions);

for (int i = 0; i < num_actions; i++) {
    printf("Action: %d, Min: %d, Max: %d\n", 
           actions[i].type, 
           actions[i].min_amount, 
           actions[i].max_amount);
}
```

## Action Types

```c
typedef enum {
    ACTION_FOLD = 0,    // Fold hand
    ACTION_CHECK,       // Check (no bet)
    ACTION_CALL,        // Call current bet
    ACTION_BET,         // Make initial bet
    ACTION_RAISE,       // Raise current bet
    ACTION_ALL_IN       // Go all-in
} ActionType;
```

### Action Rules

- **FOLD**: Always legal
- **CHECK**: Only legal when no bet to call
- **CALL**: Only legal when there's a bet to call
- **BET**: Only legal when no current bet exists, amount >= big blind
- **RAISE**: Only legal when bet exists, amount >= current_bet + min_raise
- **ALL_IN**: Always legal if player has chips

## Player States

```c
typedef enum {
    PLAYER_STATE_EMPTY = 0,      // Seat is empty
    PLAYER_STATE_WAITING,         // Waiting for next hand
    PLAYER_STATE_ACTIVE,          // Active in current hand
    PLAYER_STATE_FOLDED,          // Folded this hand
    PLAYER_STATE_ALL_IN,          // All-in
    PLAYER_STATE_SITTING_OUT      // Sitting out
} PlayerState;
```

## Betting Rounds

```c
typedef enum {
    BETTING_ROUND_PREFLOP = 0,   // Before flop
    BETTING_ROUND_FLOP,          // After flop (3 cards)
    BETTING_ROUND_TURN,          // After turn (4th card)
    BETTING_ROUND_RIVER,         // After river (5th card)
    BETTING_ROUND_SHOWDOWN,      // Showdown
    BETTING_ROUND_COMPLETE       // Hand complete
} BettingRound;
```

## Complete Example

```c
#include "game_engine.h"
#include <stdio.h>

int main() {
    // Create game
    GameState *game = game_state_create(1, 6, 10, 20);
    
    // Add 3 players
    game_add_player(game, 101, "Alice", 0, 1000);
    game_add_player(game, 102, "Bob", 2, 1000);
    game_add_player(game, 103, "Charlie", 4, 1000);
    
    // Start hand
    game_start_hand(game);
    printf("Hand %u started\n", game->hand_id);
    printf("Dealer at seat %d\n", game->dealer_seat);
    printf("Active player at seat %d\n", game->active_seat);
    
    // Game loop
    while (game->hand_in_progress) {
        int active_player_id = game->players[game->active_seat].player_id;
        
        // Get available actions
        AvailableAction actions[10];
        int num_actions;
        game_get_available_actions(game, active_player_id, actions, &num_actions);
        
        printf("\nPlayer %d's turn\n", active_player_id);
        printf("Available actions: %d\n", num_actions);
        
        // Example: Player calls
        Action action = {ACTION_CALL, 0};
        ActionValidation validation = game_validate_action(game, active_player_id, &action);
        
        if (validation.is_valid) {
            game_process_action(game, active_player_id, &action);
            printf("Player %d called\n", active_player_id);
        }
        
        // Check if only one player left
        if (game_count_active_players(game) <= 1) {
            break;
        }
    }
    
    // End hand
    game_end_hand(game);
    printf("Hand complete\n");
    
    // Clean up
    game_state_destroy(game);
    
    return 0;
}
```

## Building

### As Part of Main Project

The library is automatically built when you build the main project:

```bash
cd server
./build_all.sh
```

### Building Standalone

```bash
cd server/lib/pokergame
mkdir -p build
cd build
cmake ..
make
```

### Running Tests

```bash
cd server/lib/pokergame
clang -Wall -I./include -I../card/include -I../utils \
    test/game_engine_test.c \
    build/libCardio_pokergame.a \
    ../card/build/libCardio_card.a \
    -o build/game_engine_test

./build/game_engine_test
```

## Integration with Protocol

The game engine is designed to work with the Nuoa protocol (see [PROTOCOL.md](../../../PROTOCOL.md)).

### Example Integration

```c
void handle_action_request(conn_data_t *conn, Packet *packet) {
    // Decode action request
    ActionRequest *req = decode_action_request(packet->data);
    
    // Get game state
    GameState *game = get_game_by_id(req->game_id);
    
    // Process action
    Action action = {
        .type = parse_action_type(req->action_type),
        .amount = req->amount
    };
    
    int result = game_process_action(game, conn->user_id, &action);
    
    // Send result
    send_action_result(conn->fd, result, req->client_seq);
    
    // Broadcast update bundle if successful
    if (result == 0) {
        UpdateBundle *bundle = generate_update_bundle(game);
        broadcast_to_table(req->game_id, bundle);
    }
}
```

## Error Handling

All functions return error codes or NULL on failure. Always check return values:

```c
GameState *game = game_state_create(1, 6, 10, 20);
if (!game) {
    fprintf(stderr, "Failed to create game\n");
    return -1;
}

int result = game_add_player(game, 101, "Alice", 0, 1000);
if (result != 0) {
    fprintf(stderr, "Failed to add player: %d\n", result);
}
```

## Thread Safety

The game engine is **not thread-safe**. If using in a multi-threaded environment:

1. Use one game instance per table
2. Synchronize access with mutexes
3. Process actions serially per table

```c
pthread_mutex_t game_mutex;
pthread_mutex_lock(&game_mutex);
game_process_action(game, player_id, &action);
pthread_mutex_unlock(&game_mutex);
```

## Performance

- **Memory**: ~2-3 KB per game state
- **Action Processing**: O(1) for validation, O(n) for round completion
- **Pot Distribution**: O(n) where n = number of players
- Optimized for low latency and minimal allocations

## Debugging

Enable detailed logging by modifying the game engine or adding debug output:

```c
void debug_game_state(GameState *state) {
    printf("=== Game State ===\n");
    printf("Game ID: %d, Hand ID: %u, Seq: %u\n", 
           state->game_id, state->hand_id, state->seq);
    printf("Round: %d, Pot: %d\n", 
           state->betting_round, game_get_pot_total(state));
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        GamePlayer *p = &state->players[i];
        if (p->state != PLAYER_STATE_EMPTY) {
            printf("Seat %d: %s, Chips: %d, Bet: %d, State: %d\n",
                   i, p->name, p->money, p->bet, p->state);
        }
    }
}
```

## Limitations and Future Work

Current implementation:
- ✅ Texas Hold'em rules
- ✅ 2-9 players
- ✅ No-limit betting
- ✅ Basic pot management
- ⚠️ Simplified hand evaluation (will be enhanced)
- ⚠️ Side pot calculation (basic implementation)

Future enhancements:
- Complete side pot algorithm for multiple all-ins
- Full 7-card hand evaluation (2 hole + 5 community)
- Pot-limit and fixed-limit betting structures
- Tournament support (blind increases, etc.)
- Other poker variants (Omaha, Seven-Card Stud)

## References

- [PROTOCOL.md](../../../PROTOCOL.md) - Complete protocol specification
- [IMPLEMENTATION.md](../../../IMPLEMENTATION.md) - Architecture documentation
- [game_engine.h](include/game_engine.h) - API documentation
- [game_engine_test.c](test/game_engine_test.c) - Test examples

## License

See repository LICENSE file.

## Contributing

Contributions welcome! Please follow the existing code style and add tests for new features.
