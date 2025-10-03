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
    shuffle(state->deck, 1000);
    
    state->hand_in_progress = false;
    state->betting_round = BETTING_ROUND_COMPLETE;
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
        if (state->players[i].state == PLAYER_STATE_ACTIVE || 
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
    
    // Set dealer button
    game_set_dealer_button(state);
    
    // Set blind positions
    game_set_blinds_positions(state);
    
    // Set all waiting players to active
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->players[i].state == PLAYER_STATE_WAITING && state->players[i].money > 0) {
            state->players[i].state = PLAYER_STATE_ACTIVE;
        }
    }
    
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
    state->active_seat = game_get_next_active_seat(state, bb_seat);
    
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
    
    // Reset betting state
    state->current_bet = 0;
    state->min_raise = state->big_blind;
    state->last_aggressor_seat = -1;
    state->players_acted = 0;
    
    // Advance round
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
    int acted_count = 0;
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
    }
    
    // Round is complete if only one player left or all active players matched bet
    if (active_count <= 1) return true;
    if (all_matched && state->last_aggressor_seat != -1) return true;
    
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
    
    winner->money += state->main_pot.amount;
    state->main_pot.amount = 0;
    
    return 0;
}

// ===== Showdown =====

int game_determine_winner(GameState *state) {
    if (!state) return -1;
    
    int best_seat = -1;
    int best_value = -1;
    
    // Find active or all-in players and compare hands
    for (int i = 0; i < MAX_PLAYERS; i++) {
        GamePlayer *p = &state->players[i];
        if (p->state == PLAYER_STATE_ACTIVE || p->state == PLAYER_STATE_ALL_IN) {
            // Create temporary hand with hole cards and community cards
            Hand temp_hand;
            hand_init(&temp_hand);
            
            if (p->hole_cards[0]) add_card(&temp_hand, p->hole_cards[0]);
            if (p->hole_cards[1]) add_card(&temp_hand, p->hole_cards[1]);
            
            // For simplified version, just use hand value
            // In full implementation, would combine with community cards
            int hand_val = hand_value(&temp_hand);
            
            if (hand_val > best_value) {
                best_value = hand_val;
                best_seat = i;
            }
            
            hand_destroy(&temp_hand);
        }
    }
    
    if (best_seat >= 0) {
        game_distribute_pot(state, best_seat);
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
