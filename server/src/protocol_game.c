#include "main.h"
#include "game_engine.h"
#include "card.h"
#include <time.h>

// Decode action request from client
ActionRequest* decode_action_request(char* payload)
{
    if (!payload) {
        return NULL;
    }
    
    ActionRequest* req = malloc(sizeof(ActionRequest));
    if (!req) {
        return NULL;
    }
    
    memset(req, 0, sizeof(ActionRequest));
    
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, payload, 4096);
    
    // Read the main map
    uint32_t map_count = mpack_expect_map(&reader);
    
    for (uint32_t i = 0; i < map_count; i++) {
        char key[32];
        mpack_expect_cstr(&reader, key, sizeof(key));
        
        if (strcmp(key, "game_id") == 0) {
            req->game_id = mpack_expect_int(&reader);
        } 
        else if (strcmp(key, "action") == 0) {
            // Read the action sub-map
            uint32_t action_map_count = mpack_expect_map(&reader);
            for (uint32_t j = 0; j < action_map_count; j++) {
                char action_key[32];
                mpack_expect_cstr(&reader, action_key, sizeof(action_key));
                
                if (strcmp(action_key, "type") == 0) {
                    mpack_expect_cstr(&reader, req->action_type, sizeof(req->action_type));
                } 
                else if (strcmp(action_key, "amount") == 0) {
                    req->amount = mpack_expect_int(&reader);
                }
                else {
                    mpack_discard(&reader);
                }
            }
        } 
        else if (strcmp(key, "client_seq") == 0) {
            req->client_seq = mpack_expect_u32(&reader);
        }
        else {
            mpack_discard(&reader);
        }
    }
    
    if (mpack_reader_destroy(&reader) != mpack_ok) {
        free(req);
        return NULL;
    }
    
    return req;
}

// Encode action result response
RawBytes* encode_action_result(ActionResult* result)
{
    if (!result) {
        return NULL;
    }
    
    char* buffer = malloc(512);
    if (!buffer) {
        return NULL;
    }
    
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, 512);
    
    mpack_start_map(&writer, result->reason[0] ? 3 : 2);
    
    mpack_write_cstr(&writer, "result");
    mpack_write_int(&writer, result->result);
    
    mpack_write_cstr(&writer, "client_seq");
    mpack_write_u32(&writer, result->client_seq);
    
    if (result->reason[0]) {
        mpack_write_cstr(&writer, "reason");
        mpack_write_cstr(&writer, result->reason);
    }
    
    mpack_finish_map(&writer);
    
    size_t size = mpack_writer_buffer_used(&writer);
    mpack_error_t error = mpack_writer_destroy(&writer);
    
    if (error != mpack_ok) {
        free(buffer);
        return NULL;
    }
    
    RawBytes* raw = malloc(sizeof(RawBytes));
    if (!raw) {
        free(buffer);
        return NULL;
    }
    
    raw->data = buffer;
    raw->len = size;
    return raw;
}

// Helper: Encode a card to integer representation
static int encode_card(Card* card)
{
    if (!card) {
        return -1;  // Hidden card
    }
    // Card encoding: suit * 13 + (rank - 2)
    // Suits: 1=Spades, 2=Hearts, 3=Diamonds, 4=Clubs
    // Ranks: 2-14 (where 14=Ace, but stored as 1 in Card struct)
    int rank = card->rank;
    if (rank == 1) rank = 14;  // Ace
    return card->suit * 13 + (rank - 2);
}

// Encode full game state
RawBytes* encode_game_state(GameState* state, int viewer_player_id)
{
    if (!state) {
        return NULL;
    }
    
    char* buffer = malloc(16384);  // Large buffer for full game state
    if (!buffer) {
        return NULL;
    }
    
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, 16384);
    
    mpack_start_map(&writer, 18);
    
    // Game identification
    mpack_write_cstr(&writer, "game_id");
    mpack_write_int(&writer, state->game_id);
    
    mpack_write_cstr(&writer, "hand_id");
    mpack_write_u32(&writer, state->hand_id);
    
    mpack_write_cstr(&writer, "seq");
    mpack_write_u32(&writer, state->seq);
    
    // Game configuration
    mpack_write_cstr(&writer, "max_players");
    mpack_write_int(&writer, state->max_players);
    
    mpack_write_cstr(&writer, "small_blind");
    mpack_write_int(&writer, state->small_blind);
    
    mpack_write_cstr(&writer, "big_blind");
    mpack_write_int(&writer, state->big_blind);
    
    mpack_write_cstr(&writer, "min_buy_in");
    mpack_write_int(&writer, state->min_buy_in);
    
    mpack_write_cstr(&writer, "max_buy_in");
    mpack_write_int(&writer, state->max_buy_in);
    
    // Betting round
    mpack_write_cstr(&writer, "betting_round");
    const char* round_names[] = {"preflop", "flop", "turn", "river", "showdown", "complete"};
    mpack_write_cstr(&writer, round_names[state->betting_round]);
    
    mpack_write_cstr(&writer, "dealer_seat");
    mpack_write_int(&writer, state->dealer_seat);
    
    mpack_write_cstr(&writer, "active_seat");
    mpack_write_int(&writer, state->active_seat);
    
    // Players array
    mpack_write_cstr(&writer, "players");
    mpack_start_array(&writer, MAX_PLAYERS);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        GamePlayer* p = &state->players[i];
        
        if (p->state == PLAYER_STATE_EMPTY) {
            mpack_write_nil(&writer);
            continue;
        }
        
        mpack_start_map(&writer, 12);
        
        mpack_write_cstr(&writer, "player_id");
        mpack_write_int(&writer, p->player_id);
        
        mpack_write_cstr(&writer, "name");
        mpack_write_cstr(&writer, p->name);
        
        mpack_write_cstr(&writer, "seat");
        mpack_write_int(&writer, p->seat);
        
        mpack_write_cstr(&writer, "state");
        const char* state_names[] = {"empty", "waiting", "active", "folded", "all_in", "sitting_out"};
        mpack_write_cstr(&writer, state_names[p->state]);
        
        mpack_write_cstr(&writer, "money");
        mpack_write_int(&writer, p->money);
        
        mpack_write_cstr(&writer, "bet");
        mpack_write_int(&writer, p->bet);
        
        mpack_write_cstr(&writer, "total_bet");
        mpack_write_int(&writer, p->total_bet);
        
        // Cards - show only to the player themselves or during showdown
        mpack_write_cstr(&writer, "cards");
        mpack_start_array(&writer, 2);
        if (p->player_id == viewer_player_id || state->betting_round == BETTING_ROUND_SHOWDOWN) {
            mpack_write_int(&writer, encode_card(p->hole_cards[0]));
            mpack_write_int(&writer, encode_card(p->hole_cards[1]));
        } else {
            mpack_write_int(&writer, -1);
            mpack_write_int(&writer, -1);
        }
        mpack_finish_array(&writer);
        
        mpack_write_cstr(&writer, "is_dealer");
        mpack_write_bool(&writer, p->is_dealer);
        
        mpack_write_cstr(&writer, "is_small_blind");
        mpack_write_bool(&writer, p->is_small_blind);
        
        mpack_write_cstr(&writer, "is_big_blind");
        mpack_write_bool(&writer, p->is_big_blind);
        
        mpack_write_cstr(&writer, "timer_deadline");
        mpack_write_u64(&writer, p->timer_deadline);
        
        mpack_finish_map(&writer);
    }
    mpack_finish_array(&writer);
    
    // Community cards
    mpack_write_cstr(&writer, "community_cards");
    mpack_start_array(&writer, state->num_community_cards);
    for (int i = 0; i < state->num_community_cards; i++) {
        mpack_write_int(&writer, encode_card(state->community_cards[i]));
    }
    mpack_finish_array(&writer);
    
    // Debug logging
    if (mpack_writer_error(&writer) != mpack_ok) {
        fprintf(stderr, "ERROR after community_cards: %d (num=%d)\n", mpack_writer_error(&writer), state->num_community_cards);
    }
    
    // Pot information
    mpack_write_cstr(&writer, "main_pot");
    mpack_write_int(&writer, state->main_pot.amount);
    
    mpack_write_cstr(&writer, "side_pots");
    mpack_start_array(&writer, state->num_side_pots);
    for (int i = 0; i < state->num_side_pots; i++) {
        mpack_start_map(&writer, 2);
        
        mpack_write_cstr(&writer, "amount");
        mpack_write_int(&writer, state->side_pots[i].amount);
        
        mpack_write_cstr(&writer, "eligible_players");
        mpack_start_array(&writer, state->side_pots[i].num_players);
        for (int j = 0; j < state->side_pots[i].num_players; j++) {
            mpack_write_int(&writer, state->side_pots[i].player_ids[j]);
        }
        mpack_finish_array(&writer);
        
        mpack_finish_map(&writer);
    }
    mpack_finish_array(&writer);
    
    // Betting state
    mpack_write_cstr(&writer, "current_bet");
    mpack_write_int(&writer, state->current_bet);
    
    mpack_write_cstr(&writer, "min_raise");
    mpack_write_int(&writer, state->min_raise);
    
    // Available actions (only for the active player)
    mpack_write_cstr(&writer, "available_actions");
    if (state->active_seat >= 0 && 
        state->players[state->active_seat].player_id == viewer_player_id) {
        AvailableAction actions[10];
        int num_actions = 0;
        game_get_available_actions(state, viewer_player_id, actions, &num_actions);
        
        mpack_start_array(&writer, num_actions);
        for (int i = 0; i < num_actions; i++) {
            mpack_start_map(&writer, 4);
            
            mpack_write_cstr(&writer, "type");
            const char* action_names[] = {"fold", "check", "call", "bet", "raise", "all_in"};
            mpack_write_cstr(&writer, action_names[actions[i].type]);
            
            mpack_write_cstr(&writer, "min_amount");
            mpack_write_int(&writer, actions[i].min_amount);
            
            mpack_write_cstr(&writer, "max_amount");
            mpack_write_int(&writer, actions[i].max_amount);
            
            mpack_write_cstr(&writer, "increment");
            mpack_write_int(&writer, actions[i].increment);
            
            mpack_finish_map(&writer);
        }
        mpack_finish_array(&writer);
    } else {
        mpack_start_array(&writer, 0);
        mpack_finish_array(&writer);
    }
    
    mpack_finish_map(&writer);
    
    size_t size = mpack_writer_buffer_used(&writer);
    mpack_error_t error = mpack_writer_destroy(&writer);
    
    if (error != mpack_ok) {
        free(buffer);
        return NULL;
    }
    
    RawBytes* raw = malloc(sizeof(RawBytes));
    if (!raw) {
        free(buffer);
        return NULL;
    }
    
    raw->data = buffer;
    raw->len = size;
    return raw;
}

// Simple update bundle encoder (for now, we'll just send full game state)
// TODO: Implement incremental updates
RawBytes* encode_update_bundle(uint32_t seq, const char** notifications, int num_notifications,
                                const char** updates, int num_updates)
{
    char* buffer = malloc(4096);
    if (!buffer) {
        return NULL;
    }
    
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, 4096);
    
    mpack_start_map(&writer, 3);
    
    mpack_write_cstr(&writer, "seq");
    mpack_write_u32(&writer, seq);
    
    mpack_write_cstr(&writer, "notifications");
    mpack_start_array(&writer, num_notifications);
    // For now, empty - will be implemented later
    mpack_finish_array(&writer);
    
    mpack_write_cstr(&writer, "updates");
    mpack_start_array(&writer, num_updates);
    // For now, empty - will be implemented later
    mpack_finish_array(&writer);
    
    mpack_finish_map(&writer);
    
    size_t size = mpack_writer_buffer_used(&writer);
    mpack_error_t error = mpack_writer_destroy(&writer);
    
    if (error != mpack_ok) {
        free(buffer);
        return NULL;
    }
    
    RawBytes* raw = malloc(sizeof(RawBytes));
    if (!raw) {
        free(buffer);
        return NULL;
    }
    
    raw->data = buffer;
    raw->len = size;
    return raw;
}
