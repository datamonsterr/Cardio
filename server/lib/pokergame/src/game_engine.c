#include "game_engine.h"
#include <string.h>
#include <time.h>

// ===== Game State Management =====

GameState* game_state_create(int game_id, int max_players, int small_blind, int big_blind) {
    GameState *state = (GameState *)calloc(1, sizeof(GameState));
    if (!state) return NULL;
    
    state->game_id = game_id;
    state->max_players = (max_players > MAX_PLAYERS) ? MAX_PLAYERS : max_players;
    state->small_blind = small_blind;
    state->big_blind = big_blind;
    state->min_buy_in = big_blind * 20;
    state->max_buy_in = big_blind * 100;
    state->seq = 0;
    state->hand_id = 0;
    
    // Initialize deck
    state->deck = (Deck *)malloc(sizeof(Deck));
    if (!state->deck) {
        free(state);
        return NULL;
    }
    deck_init(state->deck);
    
    // Initialize all players as empty
    for (int i = 0; i < MAX_PLAYERS; i++) {
        state->players[i].state = PLAYER_STATE_EMPTY;
        state->players[i].seat = i;
        state->players[i].hole_cards[0] = NULL;
        state->players[i].hole_cards[1] = NULL;
    }
    
    state->num_players = 0;
    state->hand_in_progress = false;
    state->waiting_for_players = true;
    state->dealer_seat = -1;
    state->active_seat = -1;
    state->betting_round = BETTING_ROUND_COMPLETE;
    state->winner_seat = -1;
    state->amount_won = 0;
    state->winner_hand_rank = -1;
    
    return state;
}

void game_state_destroy(GameState *state) {
    if (!state) return;
    
    if (state->deck) {
        deck_destroy(state->deck);
    }
    
    free(state);
}

void game_state_reset_for_new_hand(GameState *state) {
    if (!state) return;
    
    // Reset community cards
    for (int i = 0; i < MAX_COMMUNITY_CARDS; i++) {
        state->community_cards[i] = NULL;
    }
    state->num_community_cards = 0;
    
    // Reset pots
    state->main_pot.amount = 0;
    state->main_pot.num_players = 0;
    state->num_side_pots = 0;
    
    // Reset betting state
    state->current_bet = 0;
    state->min_raise = state->big_blind;
    state->last_aggressor_seat = -1;
    state->players_acted = 0;
    
    // Reset player states
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].state != PLAYER_STATE_EMPTY && 
            state->players[i].state != PLAYER_STATE_SITTING_OUT) {
            state->players[i].state = PLAYER_STATE_WAITING;
            state->players[i].bet = 0;
            state->players[i].total_bet = 0;
            state->players[i].hole_cards[0] = NULL;
            state->players[i].hole_cards[1] = NULL;
            state->players[i].is_dealer = false;
            state->players[i].is_small_blind = false;
            state->players[i].is_big_blind = false;
        }
    }
    
    // Reset and shuffle deck
    enqueue_deck(state->deck);
    deck_fill(state->deck);
    // Re-seed random for additional randomness per hand
    srand((unsigned int)(time(NULL) + state->hand_id * 1000));
    shuffle(state->deck, 1000);
    
    state->hand_in_progress = false;
    state->betting_round = BETTING_ROUND_COMPLETE;
    state->winner_seat = -1;
    state->amount_won = 0;
    state->winner_hand_rank = -1;
}

// ===== Player Management =====

int game_add_player(GameState *state, int player_id, const char *name, int seat, int buy_in) {
    if (!state || seat < 0 || seat >= state->max_players) return -1;
    if (state->players[seat].state != PLAYER_STATE_EMPTY) return -2; // Seat occupied
    if (buy_in < state->min_buy_in || buy_in > state->max_buy_in) return -3; // Invalid buy-in
    
    state->players[seat].player_id = player_id;
    strncpy(state->players[seat].name, name, 31);
    state->players[seat].name[31] = '\0';
    state->players[seat].seat = seat;
    state->players[seat].state = PLAYER_STATE_WAITING;
    state->players[seat].money = buy_in;
    state->players[seat].bet = 0;
    state->players[seat].total_bet = 0;
    state->players[seat].hole_cards[0] = NULL;
    state->players[seat].hole_cards[1] = NULL;
    state->players[seat].is_bot = false;
    state->players[seat].original_user_id = 0;
    
    state->num_players++;
    
    // Check if we have enough players to start
    if (state->num_players >= 2) {
        state->waiting_for_players = false;
    }
    
    return 0;
}

int game_remove_player(GameState *state, int seat) {
    if (!state || seat < 0 || seat >= MAX_PLAYERS) return -1;
    if (state->players[seat].state == PLAYER_STATE_EMPTY) return -2;
    
    state->players[seat].state = PLAYER_STATE_EMPTY;
    state->players[seat].player_id = 0;
    state->players[seat].money = 0;
    state->num_players--;
    
    return 0;
}

int game_convert_player_to_bot(GameState *state, int seat) {
    if (!state || seat < 0 || seat >= MAX_PLAYERS) return -1;
    if (state->players[seat].state == PLAYER_STATE_EMPTY) return -2;
    
    GamePlayer *player = &state->players[seat];
    
    // Store original user_id for chip return later
    player->original_user_id = player->player_id;
    
    // Mark as bot and rename
    player->is_bot = true;
    strncpy(player->name, "Bot", 31);
    player->name[31] = '\0';
    player->player_id = -1; // Use -1 to indicate bot (no real player_id)
    
    // Keep their current state (ACTIVE, FOLDED, ALL_IN, etc.) and chips
    // This allows the bot to continue playing the current hand
    
    return 0;
}

GamePlayer* game_get_player_by_id(GameState *state, int player_id) {
    if (!state) return NULL;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].state != PLAYER_STATE_EMPTY && 
            state->players[i].player_id == player_id) {
            return &state->players[i];
        }
    }
    return NULL;
}

GamePlayer* game_get_player_by_seat(GameState *state, int seat) {
    if (!state || seat < 0 || seat >= MAX_PLAYERS) return NULL;
    if (state->players[seat].state == PLAYER_STATE_EMPTY) return NULL;
    return &state->players[seat];
}

int game_get_next_active_seat(GameState *state, int current_seat) {
    if (!state) return -1;
    
    int seat = current_seat;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        seat = (seat + 1) % state->max_players;
        GamePlayer *p = &state->players[seat];
        if (p->state == PLAYER_STATE_ACTIVE) {
            return seat;
        }
    }
    return -1;
}

int game_count_active_players(GameState *state) {
    if (!state) return 0;
    
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Count players who are in a state where they can participate in a game
        // WAITING = ready for next hand, ACTIVE/ALL_IN = in current hand
        if (state->players[i].state == PLAYER_STATE_WAITING ||
            state->players[i].state == PLAYER_STATE_ACTIVE || 
            state->players[i].state == PLAYER_STATE_ALL_IN) {
            count++;
        }
    }
    return count;
}

// ===== Hand Lifecycle =====

int game_start_hand(GameState *state) {
    if (!state || state->hand_in_progress) return -1;
    if (state->num_players < 2) return -2;
    
    // Reset for new hand
    game_state_reset_for_new_hand(state);
    
    state->hand_id++;
    state->seq = 0;
    state->hand_in_progress = true;
    state->winner_seat = -1;  // No winner yet
    state->amount_won = 0;    // No winnings yet
    
    // Set all waiting players to active first
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].state == PLAYER_STATE_WAITING && state->players[i].money > 0) {
            state->players[i].state = PLAYER_STATE_ACTIVE;
        }
    }
    
    // Set dealer button
    game_set_dealer_button(state);
    
    // Set blind positions (after players are active)
    game_set_blinds_positions(state);
    
    // Deal hole cards
    game_deal_hole_cards(state);
    
    // Post blinds
    game_post_blinds(state);
    
    // Set betting round to preflop
    state->betting_round = BETTING_ROUND_PREFLOP;
    
    // Find first player to act (after big blind)
    int bb_seat = -1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].is_big_blind) {
            bb_seat = i;
            break;
        }
    }
    
    if (bb_seat == -1) {
        // No big blind found - this should not happen
        state->active_seat = -1;
        return -3;
    }
    
    state->active_seat = game_get_next_active_seat(state, bb_seat);
    
    if (state->active_seat == -1) {
        // Could not find active player after big blind - this should not happen
        return -4;
    }
    
    return 0;
}

int game_deal_hole_cards(GameState *state) {
    if (!state || !state->deck) return -1;
    
    // Deal 2 cards to each active player
    for (int card_num = 0; card_num < 2; card_num++) {
        for (int seat = 0; seat < MAX_PLAYERS; seat++) {
            if (state->players[seat].state == PLAYER_STATE_ACTIVE) {
                Card *card = NULL;
                if (dequeue_card(state->deck, &card) == 0) {
                    state->players[seat].hole_cards[card_num] = card;
                }
            }
        }
    }
    
    return 0;
}

int game_deal_flop(GameState *state) {
    if (!state || !state->deck) return -1;
    if (state->num_community_cards != 0) return -2;
    
    // Burn one card
    Card *burn = NULL;
    dequeue_card(state->deck, &burn);
    
    // Deal 3 cards
    for (int i = 0; i < 3; i++) {
        Card *card = NULL;
        if (dequeue_card(state->deck, &card) == 0) {
            state->community_cards[state->num_community_cards++] = card;
        }
    }
    
    return 0;
}

int game_deal_turn(GameState *state) {
    if (!state || !state->deck) return -1;
    if (state->num_community_cards != 3) return -2;
    
    // Burn one card
    Card *burn = NULL;
    dequeue_card(state->deck, &burn);
    
    // Deal 1 card
    Card *card = NULL;
    if (dequeue_card(state->deck, &card) == 0) {
        state->community_cards[state->num_community_cards++] = card;
    }
    
    return 0;
}

int game_deal_river(GameState *state) {
    if (!state || !state->deck) return -1;
    if (state->num_community_cards != 4) return -2;
    
    // Burn one card
    Card *burn = NULL;
    dequeue_card(state->deck, &burn);
    
    // Deal 1 card
    Card *card = NULL;
    if (dequeue_card(state->deck, &card) == 0) {
        state->community_cards[state->num_community_cards++] = card;
    }
    
    return 0;
}

int game_end_hand(GameState *state) {
    if (!state) return -1;
    
    state->hand_in_progress = false;
    state->betting_round = BETTING_ROUND_COMPLETE;
    
    // Determine winner and distribute pot
    game_determine_winner(state);
    
    return 0;
}

// ===== Betting Logic =====

int game_post_blinds(GameState *state) {
    if (!state) return -1;
    
    // Find small and big blind players
    GamePlayer *sb_player = NULL;
    GamePlayer *bb_player = NULL;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].is_small_blind) {
            sb_player = &state->players[i];
        }
        if (state->players[i].is_big_blind) {
            bb_player = &state->players[i];
        }
    }
    
    // Post small blind
    if (sb_player) {
        int sb_amount = (sb_player->money >= state->small_blind) ? state->small_blind : sb_player->money;
        sb_player->bet = sb_amount;
        sb_player->total_bet = sb_amount;
        sb_player->money -= sb_amount;
        if (sb_player->money == 0) {
            sb_player->state = PLAYER_STATE_ALL_IN;
        }
    }
    
    // Post big blind
    if (bb_player) {
        int bb_amount = (bb_player->money >= state->big_blind) ? state->big_blind : bb_player->money;
        bb_player->bet = bb_amount;
        bb_player->total_bet = bb_amount;
        bb_player->money -= bb_amount;
        state->current_bet = bb_amount;
        if (bb_player->money == 0) {
            bb_player->state = PLAYER_STATE_ALL_IN;
        }
    }
    
    return 0;
}

int game_advance_betting_round(GameState *state) {
    if (!state) return -1;
    
    // Collect bets into pot
    game_collect_bets_to_pot(state);
    
    // Check how many players remain (active or all-in)
    int active_count = 0;
    int all_in_count = 0;
    int last_active_seat = -1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].state == PLAYER_STATE_ACTIVE) {
            active_count++;
            last_active_seat = i;
        }
        if (state->players[i].state == PLAYER_STATE_ALL_IN) {
            all_in_count++;
            if (last_active_seat < 0) {
                last_active_seat = i;
            }
        }
    }
    
    // If only one player remains, they win immediately
    if (active_count + all_in_count <= 1 && last_active_seat >= 0) {
        game_distribute_pot(state, last_active_seat);
        state->winner_seat = last_active_seat;
        state->betting_round = BETTING_ROUND_COMPLETE;
        state->hand_in_progress = false;
        state->active_seat = -1;
        return 0;
    }
    
    // If all remaining players are all-in (no one can act),
    // automatically deal all remaining community cards and go to showdown.
    if (active_count == 0 && all_in_count >= 2) {
        switch (state->betting_round) {
            case BETTING_ROUND_PREFLOP:
                // Deal flop, turn, and river
                game_deal_flop(state);
                game_deal_turn(state);
                game_deal_river(state);
                break;
            case BETTING_ROUND_FLOP:
                // Deal turn and river
                game_deal_turn(state);
                game_deal_river(state);
                break;
            case BETTING_ROUND_TURN:
                // Deal river
                game_deal_river(state);
                break;
            case BETTING_ROUND_RIVER:
                // Already at river, will go to showdown below
                break;
            default:
                break;
        }
        
        state->betting_round = BETTING_ROUND_SHOWDOWN;
        game_showdown(state);
        return 0;
    }
    
    // Reset betting state for next round
    state->current_bet = 0;
    state->min_raise = state->big_blind;
    state->last_aggressor_seat = -1;
    state->players_acted = 0;
    
    // Advance round normally
    switch (state->betting_round) {
        case BETTING_ROUND_PREFLOP:
            game_deal_flop(state);
            state->betting_round = BETTING_ROUND_FLOP;
            break;
        case BETTING_ROUND_FLOP:
            game_deal_turn(state);
            state->betting_round = BETTING_ROUND_TURN;
            break;
        case BETTING_ROUND_TURN:
            game_deal_river(state);
            state->betting_round = BETTING_ROUND_RIVER;
            break;
        case BETTING_ROUND_RIVER:
            state->betting_round = BETTING_ROUND_SHOWDOWN;
            game_showdown(state);
            return 0;
        default:
            return -1;
    }
    
    // Find first active player after dealer
    state->active_seat = game_get_next_active_seat(state, state->dealer_seat);
    
    return 0;
}

bool game_is_betting_round_complete(GameState *state) {
    if (!state) return false;
    
    int active_count = 0;
    int all_matched = true;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        GamePlayer *p = &state->players[i];
        if (p->state == PLAYER_STATE_ACTIVE) {
            active_count++;
            // Check if player has matched current bet or is all-in
            if (p->bet != state->current_bet && p->state != PLAYER_STATE_ALL_IN) {
                all_matched = false;
            }
        }
        // Count all-in players too
        if (p->state == PLAYER_STATE_ALL_IN) {
            active_count++;
        }
    }
    
    // Round is complete if only one player left
    if (active_count <= 1) return true;
    
    // Round is complete if:
    // 1. All active players have matched the current bet
    // 2. AND either someone bet/raised (last_aggressor_seat != -1) OR everyone checked (current_bet == 0 && all acted)
    if (all_matched) {
        // If there was a bet/raise, round is complete when everyone matched
        if (state->last_aggressor_seat != -1) return true;
        
        // If no bet/raise (everyone checking), need to ensure everyone has had a chance to act
        // Check if we've gone around the table (active_seat returned to first player after dealer)
        if (state->current_bet == 0 && state->players_acted >= active_count) {
            return true;
        }
    }
    
    return false;
}

int game_move_to_next_player(GameState *state) {
    if (!state) return -1;
    
    state->active_seat = game_get_next_active_seat(state, state->active_seat);
    return state->active_seat;
}

// ===== Action Processing =====

ActionValidation game_validate_action(GameState *state, int player_id, Action *action) {
    ActionValidation result = {false, "Unknown error", 0};
    
    if (!state || !action) {
        result.error_message = "Invalid parameters";
        return result;
    }
    
    GamePlayer *player = game_get_player_by_id(state, player_id);
    if (!player || player->state != PLAYER_STATE_ACTIVE) {
        result.error_message = "Player not active";
        return result;
    }
    
    if (player->seat != state->active_seat) {
        result.error_message = "Not player's turn";
        return result;
    }
    
    int amount_to_call = state->current_bet - player->bet;
    
    switch (action->type) {
        case ACTION_FOLD:
            result.is_valid = true;
            break;
            
        case ACTION_CHECK:
            if (amount_to_call > 0) {
                result.error_message = "Cannot check, must call or fold";
            } else {
                result.is_valid = true;
            }
            break;
            
        case ACTION_CALL:
            if (amount_to_call <= 0) {
                result.error_message = "Nothing to call";
            } else if (amount_to_call >= player->money) {
                // Call becomes all-in
                result.is_valid = true;
                result.adjusted_amount = player->money;
            } else {
                result.is_valid = true;
                result.adjusted_amount = amount_to_call;
            }
            break;
            
        case ACTION_BET:
            if (state->current_bet > 0) {
                result.error_message = "Cannot bet, must call or raise";
            } else if (action->amount < state->big_blind) {
                result.error_message = "Bet too small";
            } else if (action->amount > player->money) {
                result.error_message = "Insufficient chips";
            } else {
                result.is_valid = true;
                result.adjusted_amount = action->amount;
            }
            break;
            
        case ACTION_RAISE:
            if (state->current_bet == 0) {
                result.error_message = "Cannot raise, no bet to raise";
            } else {
                int min_raise_total = state->current_bet + state->min_raise;
                if (action->amount < min_raise_total) {
                    result.error_message = "Raise too small";
                } else if (action->amount > player->money + player->bet) {
                    result.error_message = "Insufficient chips";
                } else {
                    result.is_valid = true;
                    result.adjusted_amount = action->amount;
                }
            }
            break;
            
        case ACTION_ALL_IN:
            result.is_valid = true;
            result.adjusted_amount = player->money;
            break;
    }
    
    return result;
}

int game_process_action(GameState *state, int player_id, Action *action) {
    if (!state || !action) return -1;
    
    ActionValidation validation = game_validate_action(state, player_id, action);
    if (!validation.is_valid) {
        return -2;
    }
    
    GamePlayer *player = game_get_player_by_id(state, player_id);
    if (!player) return -3;
    
    state->seq++; // Increment sequence number for this action
    state->players_acted++; // Increment players acted counter
    
    switch (action->type) {
        case ACTION_FOLD:
            player->state = PLAYER_STATE_FOLDED;
            break;
            
        case ACTION_CHECK:
            // No chips movement
            break;
            
        case ACTION_CALL: {
            int call_amount = validation.adjusted_amount;
            player->bet += call_amount;
            player->total_bet += call_amount;
            player->money -= call_amount;
            if (player->money == 0) {
                player->state = PLAYER_STATE_ALL_IN;
            }
            break;
        }
            
        case ACTION_BET:
        case ACTION_RAISE: {
            int raise_amount = validation.adjusted_amount;
            int new_bet = raise_amount;
            int chips_to_add = new_bet - player->bet;
            player->bet = new_bet;
            player->total_bet += chips_to_add;
            player->money -= chips_to_add;
            
            state->current_bet = new_bet;
            state->min_raise = chips_to_add;
            state->last_aggressor_seat = player->seat;
            
            if (player->money == 0) {
                player->state = PLAYER_STATE_ALL_IN;
            }
            break;
        }
            
        case ACTION_ALL_IN: {
            int all_in_amount = player->money;
            player->bet += all_in_amount;
            player->total_bet += all_in_amount;
            player->money = 0;
            player->state = PLAYER_STATE_ALL_IN;
            
            if (player->bet > state->current_bet) {
                state->current_bet = player->bet;
                state->last_aggressor_seat = player->seat;
            }
            break;
        }
    }
    
    // Check if betting round is complete
    if (game_is_betting_round_complete(state)) {
        game_advance_betting_round(state);
    } else {
        game_move_to_next_player(state);
    }
    
    return 0;
}

void game_get_available_actions(GameState *state, int player_id, AvailableAction *actions, int *num_actions) {
    *num_actions = 0;
    if (!state || !actions) return;
    
    GamePlayer *player = game_get_player_by_id(state, player_id);
    if (!player || player->state != PLAYER_STATE_ACTIVE) return;
    if (player->seat != state->active_seat) return;
    
    int amount_to_call = state->current_bet - player->bet;
    
    // Fold is always available
    actions[(*num_actions)++] = (AvailableAction){ACTION_FOLD, 0, 0, 0};
    
    // Check available if no bet to call
    if (amount_to_call == 0) {
        actions[(*num_actions)++] = (AvailableAction){ACTION_CHECK, 0, 0, 0};
    }
    
    // Call available if there's a bet
    if (amount_to_call > 0 && amount_to_call < player->money) {
        actions[(*num_actions)++] = (AvailableAction){ACTION_CALL, amount_to_call, amount_to_call, 0};
    }
    
    // Bet/Raise available
    if (state->current_bet == 0 && player->money >= state->big_blind) {
        // Can bet
        actions[(*num_actions)++] = (AvailableAction){
            ACTION_BET, state->big_blind, player->money, state->big_blind
        };
    } else if (state->current_bet > 0) {
        int min_raise_total = state->current_bet + state->min_raise;
        if (player->money + player->bet > min_raise_total) {
            // Can raise
            actions[(*num_actions)++] = (AvailableAction){
                ACTION_RAISE, min_raise_total, player->money + player->bet, state->big_blind
            };
        }
    }
    
    // All-in always available
    if (player->money > 0) {
        actions[(*num_actions)++] = (AvailableAction){ACTION_ALL_IN, player->money, player->money, 0};
    }
}

// ===== Pot Management =====

void game_collect_bets_to_pot(GameState *state) {
    if (!state) return;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].state != PLAYER_STATE_EMPTY) {
            state->main_pot.amount += state->players[i].bet;
            state->players[i].bet = 0;
        }
    }
    
    // Calculate side pots if needed
    game_calculate_side_pots(state);
}

void game_calculate_side_pots(GameState *state) {
    if (!state) return;
    
    // TODO: Implement side pot calculation for all-in scenarios
    // This is a complex algorithm that handles multiple all-ins
    // For now, we'll use a simplified version with just main pot
}

int game_distribute_pot(GameState *state, int winning_seat) {
    if (!state) return -1;
    
    GamePlayer *winner = game_get_player_by_seat(state, winning_seat);
    if (!winner) return -2;
    
    int pot_amount = state->main_pot.amount;
    winner->money += pot_amount;
    state->amount_won = pot_amount;  // Store for clients
    state->main_pot.amount = 0;
    
    return 0;
}

// ===== Showdown =====

// Calculate best 5-card hand value from 7 cards (2 hole + 5 community)
// Returns hand value using same scale as hand_value() function
static int calculate_best_hand_value(Card* hole_cards[2], Card* community_cards[5], int num_community) {
    if (!hole_cards[0] || !hole_cards[1]) return 0;
    
    // Collect all 7 cards
    Card* all_cards[7];
    int num_cards = 2;
    all_cards[0] = hole_cards[0];
    all_cards[1] = hole_cards[1];
    
    for (int i = 0; i < num_community && i < 5; i++) {
        if (community_cards[i]) {
            all_cards[num_cards++] = community_cards[i];
        }
    }
    
    if (num_cards < 5) return 0; // Need at least 5 cards
    
    int best_value = 0;
    
    // Try all combinations of 5 cards from 7 cards (C(7,5) = 21 combinations)
    // For simplicity, we'll check all combinations
    for (int c1 = 0; c1 < num_cards; c1++) {
        for (int c2 = c1 + 1; c2 < num_cards; c2++) {
            for (int c3 = c2 + 1; c3 < num_cards; c3++) {
                for (int c4 = c3 + 1; c4 < num_cards; c4++) {
                    for (int c5 = c4 + 1; c5 < num_cards; c5++) {
                        // Count suits and ranks for this 5-card combination
                        int suits[5] = {0};
                        int ranks[15] = {0};
                        int highcard = 0;
                        
                        Card* combo[5] = {all_cards[c1], all_cards[c2], all_cards[c3], all_cards[c4], all_cards[c5]};
                        
                        for (int i = 0; i < 5; i++) {
                            int suit = combo[i]->suit;
                            int rank = combo[i]->rank;
                            if (rank == 1) rank = 14; // Ace high
                            
                            suits[suit]++;
                            ranks[rank]++;
                            if (rank > highcard) highcard = rank;
                        }
                        
                        // Check for flush
                        int flush = 0;
                        int flush_suit = 0;
                        for (int i = 1; i <= 4; i++) {
                            if (suits[i] == 5) {
                                flush = 1;
                                flush_suit = i;
                                break;
                            }
                        }
                        
                        // If flush, recalculate highcard as highest card in flush suit
                        int flush_highcard = highcard;
                        if (flush) {
                            flush_highcard = 0;
                            for (int i = 0; i < 5; i++) {
                                int rank = combo[i]->rank;
                                if (rank == 1) rank = 14; // Ace high
                                if (combo[i]->suit == flush_suit && rank > flush_highcard) {
                                    flush_highcard = rank;
                                }
                            }
                        }
                        
                        // Check for straight
                        int straight = 0;
                        int consecutive = 0;
                        int max_consecutive = 0;
                        for (int i = 2; i < 15; i++) {
                            if (ranks[i] > 0) {
                                consecutive++;
                                if (consecutive > max_consecutive) {
                                    max_consecutive = consecutive;
                                }
                                if (consecutive >= 5) {
                                    straight = 1;
                                    highcard = i; // Update highcard to end of straight
                                }
                            } else {
                                consecutive = 0;
                            }
                        }
                        // Check for A-2-3-4-5 straight (wheel)
                        if (!straight && ranks[14] > 0 && ranks[2] > 0 && ranks[3] > 0 && ranks[4] > 0 && ranks[5] > 0) {
                            straight = 1;
                            highcard = 5; // 5-high straight
                        }
                        
                        // Check for four of a kind, three of a kind, pairs
                        int four_kind = 0, three_kind = 0, pair_count = 0;
                        int four_rank = 0, three_rank = 0, pair1_rank = 0, pair2_rank = 0;
                        
                        for (int i = 2; i < 15; i++) {
                            if (ranks[i] == 4) {
                                four_kind = 1;
                                four_rank = i;
                            } else if (ranks[i] == 3) {
                                three_kind = 1;
                                three_rank = i;
                            } else if (ranks[i] == 2) {
                                pair_count++;
                                if (pair1_rank == 0) {
                                    pair1_rank = i;
                                } else {
                                    pair2_rank = i;
                                }
                            }
                        }
                        
                        // Calculate hand value (using same scale as hand_value function)
                        int hand_val = 0;
                        
                        if (flush && straight) {
                            // Straight flush
                            hand_val = 13 * 8 + flush_highcard - 1; // SFLUSH = 8
                        } else if (four_kind) {
                            // Four of a kind
                            hand_val = 13 * 7 + four_rank - 1; // FOURK = 7
                        } else if (three_kind && pair_count >= 1) {
                            // Full house
                            hand_val = 13 * 6 + three_rank - 1; // FULLHOUSE = 6
                        } else if (flush) {
                            // Flush - use flush_highcard (highest card in flush suit)
                            hand_val = 13 * 5 + flush_highcard - 1; // FLUSH = 5
                        } else if (straight) {
                            // Straight
                            hand_val = 13 * 4 + highcard - 1; // STRAIGHT = 4
                        } else if (three_kind) {
                            // Three of a kind
                            hand_val = 13 * 3 + three_rank - 1; // THREEK = 3
                        } else if (pair_count >= 2) {
                            // Two pair
                            int higher_pair = (pair1_rank > pair2_rank) ? pair1_rank : pair2_rank;
                            hand_val = 13 * 2 + higher_pair - 1; // TWOPAIR = 2
                        } else if (pair_count == 1) {
                            // Pair
                            hand_val = 13 * 1 + pair1_rank - 1; // PAIR = 1
                        } else {
                            // High card
                            hand_val = 13 * 0 + highcard - 1; // HIGHCARD = 0
                        }
                        
                        if (hand_val > best_value) {
                            best_value = hand_val;
                        }
                    }
                }
            }
        }
    }
    
    return best_value;
}

int game_determine_winner(GameState *state) {
    if (!state) return -1;
    
    int best_seat = -1;
    int best_value = -1;
    
    // Find active or all-in players and compare hands
    for (int i = 0; i < MAX_PLAYERS; i++) {
        GamePlayer *p = &state->players[i];
        if (p->state == PLAYER_STATE_ACTIVE || p->state == PLAYER_STATE_ALL_IN) {
            // Calculate best 5-card hand from 7 cards (2 hole + 5 community)
            int hand_val = calculate_best_hand_value(p->hole_cards, state->community_cards, state->num_community_cards);
            
            if (hand_val > best_value) {
                best_value = hand_val;
                best_seat = i;
            }
        }
    }
    
    if (best_seat >= 0) {
        game_distribute_pot(state, best_seat);
        state->winner_seat = best_seat;  // Store winner for clients
        // Extract hand rank from hand value (hand_val = 13 * rank + highcard - 1)
        // rank = 0 (High Card) to 8 (Straight Flush)
        state->winner_hand_rank = best_value / 13;
    }
    
    return best_seat;
}

void game_showdown(GameState *state) {
    if (!state) return;
    
    // Collect any remaining bets
    game_collect_bets_to_pot(state);
    
    // Determine and award winner
    game_determine_winner(state);
    
    state->betting_round = BETTING_ROUND_COMPLETE;
}

// ===== Utility Functions =====

void game_set_dealer_button(GameState *state) {
    if (!state) return;
    
    // Find next seated player after current dealer
    int start_seat = (state->dealer_seat >= 0) ? state->dealer_seat : -1;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        int seat = (start_seat + 1 + i) % state->max_players;
        if (state->players[seat].state != PLAYER_STATE_EMPTY && 
            state->players[seat].state != PLAYER_STATE_SITTING_OUT) {
            state->dealer_seat = seat;
            state->players[seat].is_dealer = true;
            return;
        }
    }
}

void game_set_blinds_positions(GameState *state) {
    if (!state || state->dealer_seat < 0) return;
    
    // Small blind is next active player after dealer
    int sb_seat = game_get_next_active_seat(state, state->dealer_seat);
    if (sb_seat >= 0) {
        state->players[sb_seat].is_small_blind = true;
        
        // Big blind is next active player after small blind
        int bb_seat = game_get_next_active_seat(state, sb_seat);
        if (bb_seat >= 0) {
            state->players[bb_seat].is_big_blind = true;
        }
    }
}

bool game_has_minimum_players(GameState *state) {
    if (!state) return false;
    return state->num_players >= 2;
}

int game_get_pot_total(GameState *state) {
    if (!state) return 0;
    
    int total = state->main_pot.amount;
    for (int i = 0; i < state->num_side_pots; i++) {
        total += state->side_pots[i].amount;
    }
    
    return total;
}
