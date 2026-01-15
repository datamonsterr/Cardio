#pragma once
#include "pokergame.h"
#include "card.h"
#include <stdint.h>
#include <stdbool.h>

// Maximum players at a table
#define MAX_PLAYERS 9
#define MAX_COMMUNITY_CARDS 5

// Player states
typedef enum {
    PLAYER_STATE_EMPTY = 0,       // Seat is empty
    PLAYER_STATE_WAITING,          // Player waiting for next hand
    PLAYER_STATE_ACTIVE,           // Player active in current hand
    PLAYER_STATE_FOLDED,           // Player folded this hand
    PLAYER_STATE_ALL_IN,           // Player is all-in
    PLAYER_STATE_SITTING_OUT       // Player sitting out
} PlayerState;

// Betting rounds
typedef enum {
    BETTING_ROUND_PREFLOP = 0,
    BETTING_ROUND_FLOP,
    BETTING_ROUND_TURN,
    BETTING_ROUND_RIVER,
    BETTING_ROUND_SHOWDOWN,
    BETTING_ROUND_COMPLETE
} BettingRound;

// Action types
typedef enum {
    ACTION_FOLD = 0,
    ACTION_CHECK,
    ACTION_CALL,
    ACTION_BET,
    ACTION_RAISE,
    ACTION_ALL_IN
} ActionType;

// Action structure
typedef struct {
    ActionType type;
    int amount;      // Amount for bet/raise, 0 for fold/check/call
} Action;

// Available action with constraints
typedef struct {
    ActionType type;
    int min_amount;
    int max_amount;
    int increment;   // Smallest betting unit
} AvailableAction;

// Game player (extends basic player with game state)
typedef struct {
    int player_id;              // Connection/user ID
    char name[32];              // Player name
    int seat;                   // Seat number (0-8)
    PlayerState state;          // Current state
    int money;                  // Stack size
    int bet;                    // Current bet in this round
    int total_bet;              // Total bet in this hand
    Card *hole_cards[2];        // Hole cards (NULL if not dealt)
    bool is_dealer;             // Is this player the dealer?
    bool is_small_blind;        // Is this player small blind?
    bool is_big_blind;          // Is this player big blind?
    uint64_t timer_deadline;    // Timestamp when action timer expires (epoch_ms)
} GamePlayer;

// Pot structure (for side pots)
typedef struct {
    int amount;
    int player_ids[MAX_PLAYERS]; // Players eligible for this pot
    int num_players;
} Pot;

// Game state structure
typedef struct {
    // Game identification
    int game_id;
    uint32_t hand_id;           // Current hand ID
    uint32_t seq;               // Sequence number for updates
    
    // Game configuration
    int max_players;
    int small_blind;
    int big_blind;
    int min_buy_in;
    int max_buy_in;
    
    // Game state
    BettingRound betting_round;
    int dealer_seat;            // Dealer button position
    int active_seat;            // Current player to act
    
    // Players
    GamePlayer players[MAX_PLAYERS];
    int num_players;            // Count of seated players
    
    // Community cards
    Card *community_cards[MAX_COMMUNITY_CARDS];
    int num_community_cards;
    
    // Pots
    Pot main_pot;
    Pot side_pots[MAX_PLAYERS]; // Side pots
    int num_side_pots;
    
    // Betting state
    int current_bet;            // Current bet to match
    int min_raise;              // Minimum raise amount
    int last_aggressor_seat;    // Last player to bet/raise
    int players_acted;          // Number of players who have acted this round
    
    // Deck
    Deck *deck;
    
    // Game flags
    bool hand_in_progress;
    int winner_seat;            // Winner of the hand (-1 if not determined yet)
    int amount_won;             // Amount won by the winner (0 if not determined)
    int winner_hand_rank;       // Hand rank of winner (0=High Card, 1=Pair, 2=Two Pair, 3=Three Kind, 4=Straight, 5=Flush, 6=Full House, 7=Four Kind, 8=Straight Flush)
    bool waiting_for_players;   // Waiting for minimum players
} GameState;

// Action validation result
typedef struct {
    bool is_valid;
    const char *error_message;
    int adjusted_amount;  // Adjusted amount if original was invalid
} ActionValidation;

// ===== Game State Management =====
GameState* game_state_create(int game_id, int max_players, int small_blind, int big_blind);
void game_state_destroy(GameState *state);
void game_state_reset_for_new_hand(GameState *state);

// ===== Player Management =====
int game_add_player(GameState *state, int player_id, const char *name, int seat, int buy_in);
int game_remove_player(GameState *state, int seat);
GamePlayer* game_get_player_by_id(GameState *state, int player_id);
GamePlayer* game_get_player_by_seat(GameState *state, int seat);
int game_get_next_active_seat(GameState *state, int current_seat);
int game_count_active_players(GameState *state);

// ===== Hand Lifecycle =====
int game_start_hand(GameState *state);
int game_deal_hole_cards(GameState *state);
int game_deal_flop(GameState *state);
int game_deal_turn(GameState *state);
int game_deal_river(GameState *state);
int game_end_hand(GameState *state);

// ===== Betting Logic =====
int game_post_blinds(GameState *state);
int game_advance_betting_round(GameState *state);
bool game_is_betting_round_complete(GameState *state);
int game_move_to_next_player(GameState *state);

// ===== Action Processing =====
ActionValidation game_validate_action(GameState *state, int player_id, Action *action);
int game_process_action(GameState *state, int player_id, Action *action);
void game_get_available_actions(GameState *state, int player_id, AvailableAction *actions, int *num_actions);

// ===== Pot Management =====
void game_collect_bets_to_pot(GameState *state);
void game_calculate_side_pots(GameState *state);
int game_distribute_pot(GameState *state, int winning_seat);

// ===== Showdown =====
int game_determine_winner(GameState *state);
void game_showdown(GameState *state);

// ===== Utility Functions =====
void game_set_dealer_button(GameState *state);
void game_set_blinds_positions(GameState *state);
bool game_has_minimum_players(GameState *state);
int game_get_pot_total(GameState *state);
