#include "game_engine.h"
#include "testing.h"
#include <stdio.h>
#include <assert.h>

void test_game_state_creation() {
    printf("Testing game state creation...\n");
    
    GameState *state = game_state_create(1, 6, 10, 20);
    assert(state != NULL);
    assert(state->game_id == 1);
    assert(state->max_players == 6);
    assert(state->small_blind == 10);
    assert(state->big_blind == 20);
    assert(state->num_players == 0);
    assert(state->hand_in_progress == false);
    
    game_state_destroy(state);
    printf("✓ Game state creation test passed\n");
}

void test_add_players() {
    printf("Testing player addition...\n");
    
    GameState *state = game_state_create(1, 6, 10, 20);
    
    // Add players
    int result = game_add_player(state, 101, "Alice", 0, 1000);
    assert(result == 0);
    assert(state->num_players == 1);
    
    result = game_add_player(state, 102, "Bob", 1, 1000);
    assert(result == 0);
    assert(state->num_players == 2);
    
    // Try to add player to occupied seat
    result = game_add_player(state, 103, "Charlie", 0, 1000);
    assert(result == -2);
    assert(state->num_players == 2);
    
    // Verify players
    GamePlayer *alice = game_get_player_by_id(state, 101);
    assert(alice != NULL);
    assert(alice->seat == 0);
    assert(alice->money == 1000);
    
    GamePlayer *bob = game_get_player_by_seat(state, 1);
    assert(bob != NULL);
    assert(bob->player_id == 102);
    
    game_state_destroy(state);
    printf("✓ Player addition test passed\n");
}

void test_hand_start() {
    printf("Testing hand start...\n");
    
    GameState *state = game_state_create(1, 6, 10, 20);
    
    // Add two players
    game_add_player(state, 101, "Alice", 0, 1000);
    game_add_player(state, 102, "Bob", 2, 1000);
    
    // Start hand
    int result = game_start_hand(state);
    assert(result == 0);
    assert(state->hand_in_progress == true);
    assert(state->hand_id == 1);
    assert(state->betting_round == BETTING_ROUND_PREFLOP);
    
    // Check blinds were posted
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
    
    assert(sb_player != NULL);
    assert(bb_player != NULL);
    assert(sb_player->bet == 10);
    assert(bb_player->bet == 20);
    assert(state->current_bet == 20);
    
    // Check hole cards were dealt
    assert(state->players[0].hole_cards[0] != NULL);
    assert(state->players[0].hole_cards[1] != NULL);
    assert(state->players[2].hole_cards[0] != NULL);
    assert(state->players[2].hole_cards[1] != NULL);
    
    game_state_destroy(state);
    printf("✓ Hand start test passed\n");
}

void test_action_validation() {
    printf("Testing action validation...\n");
    
    GameState *state = game_state_create(1, 6, 10, 20);
    game_add_player(state, 101, "Alice", 0, 1000);
    game_add_player(state, 102, "Bob", 2, 1000);
    game_start_hand(state);
    
    // Get active player
    int active_player_id = state->players[state->active_seat].player_id;
    
    // Test fold - always valid
    Action fold_action = {ACTION_FOLD, 0};
    ActionValidation validation = game_validate_action(state, active_player_id, &fold_action);
    assert(validation.is_valid == true);
    
    // Test call - valid when there's a bet
    Action call_action = {ACTION_CALL, 0};
    validation = game_validate_action(state, active_player_id, &call_action);
    assert(validation.is_valid == true); // Should be valid since big blind is bet
    
    // Test check when bet exists - invalid
    Action check_action = {ACTION_CHECK, 0};
    validation = game_validate_action(state, active_player_id, &check_action);
    assert(validation.is_valid == false);
    
    // Test raise with valid amount
    Action raise_action = {ACTION_RAISE, 50};
    validation = game_validate_action(state, active_player_id, &raise_action);
    assert(validation.is_valid == true);
    
    // Test bet with too small amount (before any bet)
    Action bet_action = {ACTION_BET, 5};
    validation = game_validate_action(state, active_player_id, &bet_action);
    assert(validation.is_valid == false); // Can't bet when there's already a bet
    
    game_state_destroy(state);
    printf("✓ Action validation test passed\n");
}

void test_action_processing() {
    printf("Testing action processing...\n");
    
    GameState *state = game_state_create(1, 6, 10, 20);
    game_add_player(state, 101, "Alice", 0, 1000);
    game_add_player(state, 102, "Bob", 2, 1000);
    game_start_hand(state);
    
    int active_player_id = state->players[state->active_seat].player_id;
    GamePlayer *active_player = game_get_player_by_id(state, active_player_id);
    int initial_money = active_player->money;
    
    // Process call action
    Action call_action = {ACTION_CALL, 0};
    int result = game_process_action(state, active_player_id, &call_action);
    assert(result == 0);
    
    // Check that money decreased
    assert(active_player->money < initial_money);
    
    // Check sequence number increased
    assert(state->seq > 0);
    
    game_state_destroy(state);
    printf("✓ Action processing test passed\n");
}

void test_available_actions() {
    printf("Testing available actions...\n");
    
    GameState *state = game_state_create(1, 6, 10, 20);
    game_add_player(state, 101, "Alice", 0, 1000);
    game_add_player(state, 102, "Bob", 2, 1000);
    game_start_hand(state);
    
    int active_player_id = state->players[state->active_seat].player_id;
    
    AvailableAction actions[10];
    int num_actions = 0;
    
    game_get_available_actions(state, active_player_id, actions, &num_actions);
    
    assert(num_actions > 0);
    
    // Should have fold, call, raise, and all-in at minimum
    int has_fold = 0, has_call = 0, has_raise = 0, has_all_in = 0;
    for (int i = 0; i < num_actions; i++) {
        if (actions[i].type == ACTION_FOLD) has_fold = 1;
        if (actions[i].type == ACTION_CALL) has_call = 1;
        if (actions[i].type == ACTION_RAISE) has_raise = 1;
        if (actions[i].type == ACTION_ALL_IN) has_all_in = 1;
    }
    
    assert(has_fold == 1);
    assert(has_call == 1); // Big blind is posted
    assert(has_raise == 1);
    assert(has_all_in == 1);
    
    game_state_destroy(state);
    printf("✓ Available actions test passed\n");
}

void test_pot_management() {
    printf("Testing pot management...\n");
    
    GameState *state = game_state_create(1, 6, 10, 20);
    game_add_player(state, 101, "Alice", 0, 1000);
    game_add_player(state, 102, "Bob", 2, 1000);
    game_start_hand(state);
    
    // Initial pot should be just the blinds
    int initial_pot = state->main_pot.amount;
    
    // Have players put money in
    state->players[0].bet = 50;
    state->players[2].bet = 50;
    
    // Collect bets
    game_collect_bets_to_pot(state);
    
    // Pot should have increased
    assert(state->main_pot.amount > initial_pot);
    
    // Bets should be reset
    assert(state->players[0].bet == 0);
    assert(state->players[2].bet == 0);
    
    game_state_destroy(state);
    printf("✓ Pot management test passed\n");
}

int main() {
    printf("\n=== Running Game Engine Tests ===\n\n");
    
    test_game_state_creation();
    test_add_players();
    test_hand_start();
    test_action_validation();
    test_action_processing();
    test_available_actions();
    test_pot_management();
    
    printf("\n=== All Game Engine Tests Passed! ===\n\n");
    
    return 0;
}
