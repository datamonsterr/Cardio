#include "game.h"
#include "main.h"

TableList* init_table_list(size_t capacity)
{
    TableList* table_list = malloc(sizeof(TableList));
    if (table_list == NULL)
    {
        return NULL;
    }
    table_list->tables = malloc(capacity * sizeof(Table));
    if (table_list->tables == NULL)
    {
        free(table_list);
        return NULL;
    }
    table_list->size = 0;
    table_list->capacity = capacity;
    return table_list;
}
int add_table(TableList* table_list, char* table_name, int max_player, int min_bet)
{

    if (table_list->size == table_list->capacity)
    {
        TableList* new_table_list = realloc(table_list->tables, 2 * table_list->capacity * sizeof(Table));
        if (new_table_list == NULL)
        {
            logger(MAIN_LOG, "Error", "add_table: Cannot allocate memory for table list");
            return -1;
        }
        table_list->tables = (Table*) new_table_list;
        table_list->capacity *= 2;
    }

    int id = table_list->size + 1;
    while (find_table_by_id(table_list, id) != -1)
    {
        id++;
    }
    table_list->tables[table_list->size].id = id;
    strncpy(table_list->tables[table_list->size].name, table_name, 32);
    table_list->tables[table_list->size].max_player = max_player;
    table_list->tables[table_list->size].min_bet = min_bet;
    table_list->tables[table_list->size].current_player = 0;
    
    // Initialize game state
    int small_blind = min_bet / 2;
    int big_blind = min_bet;
    table_list->tables[table_list->size].game_state = game_state_create(id, max_player, small_blind, big_blind);
    
    // Initialize connection tracking
    for (int i = 0; i < MAX_PLAYERS; i++) {
        table_list->tables[table_list->size].connections[i] = NULL;
        table_list->tables[table_list->size].seat_to_conn_idx[i] = -1;
    }
    
    // Initialize game tracking fields
    table_list->tables[table_list->size].active_seat = -1;
    table_list->tables[table_list->size].game_started = false;
    
    table_list->size++;
    return id;
}
int find_table_by_id(TableList* table_list, int id)
{
    for (int i = 0; i < table_list->size; i++)
    {
        if (table_list->tables[i].id == id)
        {
            return i;
        }
    }
    return -1;
}
int remove_table(TableList* table_list, int id)
{
    int index = find_table_by_id(table_list, id);
    if (index == -1)
    {
        return -1;
    }
    for (int i = index; i < table_list->size - 1; i++)
    {
        table_list->tables[i] = table_list->tables[i + 1];
    }
    table_list->size--;
    return 0;
}
int join_table(conn_data_t* conn_data, TableList* table_list, int table_id)
{
    int index = find_table_by_id(table_list, table_id);
    if (index == -1)
    {
        logger(MAIN_LOG, "Error", "join_table: Table not found");
        return -1;
    }
    if (table_list->tables[index].current_player >= table_list->tables[index].max_player)
    {
        logger(MAIN_LOG, "Error", "join_table: Table is full");
        return -2;
    }
    if (conn_data->table_id != 0)
    {
        logger(MAIN_LOG, "Error", "join_table: Player is already at a table");
        return -3;
    }
    
    Table* table = &table_list->tables[index];
    GameState* game_state = table->game_state;
    
    // Find an empty seat
    int seat = -1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game_state->players[i].state == PLAYER_STATE_EMPTY) {
            seat = i;
            break;
        }
    }
    
    if (seat == -1) {
        logger(MAIN_LOG, "Error", "join_table: No empty seats");
        return -2;
    }
    
    // Add player to game state with default buy-in
    int buy_in = game_state->big_blind * 50; // Default 50 big blinds
    if (conn_data->balance < buy_in) {
        buy_in = conn_data->balance;  // Use whatever they have
    }
    
    int result = game_add_player(game_state, conn_data->user_id, conn_data->username, seat, buy_in);
    if (result != 0) {
        logger(MAIN_LOG, "Error", "join_table: Failed to add player to game state");
        return -4;
    }
    
    // Track connection
    table->connections[table->current_player] = conn_data;
    table->seat_to_conn_idx[seat] = table->current_player;
    table->current_player++;
    
    conn_data->table_id = table_id;
    conn_data->seat = seat;

    char msg[256];
    sprintf(msg, "join_table: Player %s (id=%d) joined table %d at seat %d", 
            conn_data->username, conn_data->user_id, table_id, seat);
    logger(MAIN_LOG, "Info", msg);
    
    return index;
}

int leave_table(conn_data_t* conn_data, TableList* table_list)
{
    int index = find_table_by_id(table_list, conn_data->table_id);
    if (index == -1)
    {
        return -1;
    }
    
    Table* table = &table_list->tables[index];
    
    // Remove player from game state if they have a seat
    if (conn_data->seat >= 0) {
        game_remove_player(table->game_state, conn_data->seat);
        
        // Remove from connection tracking
        int conn_idx = table->seat_to_conn_idx[conn_data->seat];
        if (conn_idx >= 0 && conn_idx < table->current_player) {
            table->connections[conn_idx] = NULL;
            table->seat_to_conn_idx[conn_data->seat] = -1;
        }
    }
    
    int current_player = table->current_player;
    if (current_player > 0) {
        table->current_player--;
    }
    
    // Update game state tracking
    if (table->game_state) {
        int active_players = game_count_active_players(table->game_state);
        
        // If not enough players to continue, reset game state
        if (active_players < 2) {
            table->game_started = false;
            table->active_seat = -1;
        } else {
            // Update active_seat from game state
            table->active_seat = table->game_state->active_seat;
            
            // Broadcast updated game state to remaining players
            broadcast_game_state_to_table(table);
        }
    }
    
    // If no players left, clean up the table
    if (table->current_player == 0)
    {
        if (table->game_state) {
            game_state_destroy(table->game_state);
            table->game_state = NULL;
        }
        remove_table(table_list, conn_data->table_id);
    }
    
    conn_data->table_id = 0;
    conn_data->seat = -1;
    return 0;
}

void free_table_list(TableList* table_list)
{
    // Clean up all game states before freeing table list
    for (size_t i = 0; i < table_list->size; i++) {
        if (table_list->tables[i].game_state) {
            game_state_destroy(table_list->tables[i].game_state);
        }
    }
    free(table_list->tables);
    free(table_list);
}

// Broadcast a message to all players at a table
void broadcast_to_table(int table_id, TableList* table_list, char* data, int len)
{
    int index = find_table_by_id(table_list, table_id);
    if (index == -1) {
        logger(MAIN_LOG, "Error", "broadcast_to_table: Table not found");
        return;
    }
    
    Table* table = &table_list->tables[index];
    
    for (int i = 0; i < table->current_player; i++) {
        if (table->connections[i] != NULL && table->connections[i]->fd > 0) {
            int send_len = len;
            if (sendall(table->connections[i]->fd, data, &send_len) == -1) {
                char msg[256];
                snprintf(msg, sizeof(msg), "broadcast_to_table: Failed to send to fd=%d", 
                        table->connections[i]->fd);
                logger(MAIN_LOG, "Error", msg);
            }
        }
    }
}

// Broadcast game state to all players at a table
// Returns the number of successful broadcasts
int broadcast_game_state_to_table(Table* table)
{
    if (!table || !table->game_state) {
        logger(MAIN_LOG, "Error", "broadcast_game_state_to_table: Invalid table or game state");
        return -1;
    }
    
    GameState* gs = table->game_state;
    int successful_broadcasts = 0;
    int failed_broadcasts = 0;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Broadcasting game state (hand=%u, seq=%u) to %d players at table %d", 
             gs->hand_id, gs->seq, table->current_player, table->id);
    logger(MAIN_LOG, "Info", msg);
    
    for (int i = 0; i < table->current_player; i++) {
        if (table->connections[i] == NULL || table->connections[i]->fd <= 0) {
            snprintf(msg, sizeof(msg), "Skipping null/invalid connection at index %d", i);
            logger(MAIN_LOG, "Debug", msg);
            continue;
        }
        
        conn_data_t* conn = table->connections[i];
        
        // Encode game state for this specific player
        RawBytes* game_state_data = encode_game_state(gs, conn->user_id);
        if (!game_state_data) {
            snprintf(msg, sizeof(msg), "Failed to encode game state for user_id=%d fd=%d", 
                     conn->user_id, conn->fd);
            logger(MAIN_LOG, "Error", msg);
            failed_broadcasts++;
            continue;
        }
        
        // Wrap in packet
        RawBytes* broadcast_packet = encode_packet(PROTOCOL_V1, PACKET_UPDATE_GAMESTATE, 
                                                   game_state_data->data, game_state_data->len);
        if (!broadcast_packet) {
            snprintf(msg, sizeof(msg), "Failed to encode packet for user_id=%d fd=%d", 
                     conn->user_id, conn->fd);
            logger(MAIN_LOG, "Error", msg);
            free(game_state_data->data);
            free(game_state_data);
            failed_broadcasts++;
            continue;
        }
        
        // Send to player
        int send_len = broadcast_packet->len;
        int result = sendall(conn->fd, broadcast_packet->data, &send_len);
        
        if (result == -1) {
            snprintf(msg, sizeof(msg), "Failed to send game state to user='%s' user_id=%d fd=%d", 
                     conn->username, conn->user_id, conn->fd);
            logger(MAIN_LOG, "Error", msg);
            failed_broadcasts++;
        } else {
            snprintf(msg, sizeof(msg), "Sent game state (%d bytes) to user='%s' user_id=%d fd=%d", 
                     send_len, conn->username, conn->user_id, conn->fd);
            logger(MAIN_LOG, "Debug", msg);
            successful_broadcasts++;
        }
        
        // Clean up
        free(broadcast_packet->data);
        free(broadcast_packet);
        free(game_state_data->data);
        free(game_state_data);
    }
    
    if (failed_broadcasts > 0) {
        snprintf(msg, sizeof(msg), "Broadcast complete: %d successful, %d failed", 
                 successful_broadcasts, failed_broadcasts);
        logger(MAIN_LOG, "Warn", msg);
    } else {
        snprintf(msg, sizeof(msg), "Broadcast complete: %d players notified", successful_broadcasts);
        logger(MAIN_LOG, "Info", msg);
    }
    
    return successful_broadcasts;
}

// Start game if we have enough players
void start_game_if_ready(Table* table)
{
    if (!table || !table->game_state) {
        char msg[256];
        snprintf(msg, sizeof(msg), "start_game_if_ready: Invalid table or game_state (table=%p, game_state=%p)", 
                 (void*)table, table ? (void*)table->game_state : NULL);
        logger(MAIN_LOG, "Debug", msg);
        return;
    }
    
    GameState* gs = table->game_state;
    
    // Need at least 2 players to start
    int active_count = game_count_active_players(gs);
    if (active_count < 2) {
        char msg[256];
        snprintf(msg, sizeof(msg), "start_game_if_ready: Not enough players (count=%d, need 2) at table %d", 
                 active_count, table->id);
        logger(MAIN_LOG, "Debug", msg);
        return;
    }
    
    // Don't restart if game is in progress
    if (gs->hand_in_progress) {
        char msg[256];
        snprintf(msg, sizeof(msg), "start_game_if_ready: Hand already in progress (hand_id=%u) at table %d", 
                 gs->hand_id, table->id);
        logger(MAIN_LOG, "Debug", msg);
        return;
    }
    
    // Start a new hand
    char msg[256];
    snprintf(msg, sizeof(msg), "Starting hand %d at table %d (active_players=%d)", 
             gs->hand_id + 1, table->id, active_count);
    logger(MAIN_LOG, "Info", msg);
    
    int result = game_start_hand(gs);
    if (result != 0) {
        snprintf(msg, sizeof(msg), "start_game_if_ready: Failed to start hand (result=%d) at table %d", 
                 result, table->id);
        logger(MAIN_LOG, "Error", msg);
        return;
    }
    
    // Update table tracking fields
    table->game_started = true;
    table->active_seat = gs->active_seat;
    
    // Broadcast game state to all players at the table
    int broadcast_count = broadcast_game_state_to_table(table);
    if (broadcast_count <= 0) {
        snprintf(msg, sizeof(msg), "Warning: No players received game start broadcast at table %d", table->id);
        logger(MAIN_LOG, "Warn", msg);
    } else {
        snprintf(msg, sizeof(msg), "Successfully started hand %d at table %d, broadcast to %d players", 
                 gs->hand_id, table->id, broadcast_count);
        logger(MAIN_LOG, "Info", msg);
    }
}