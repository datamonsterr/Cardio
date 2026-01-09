#include "../include/db.h"

// Get user ID by username
int dbGetUserIdByUsername(PGconn* conn, const char* username)
{
    const char* query = "SELECT user_id FROM \"User\" WHERE username = $1 LIMIT 1";
    const char* paramValues[] = {username};

    PGresult* res = PQexecParams(conn, query,
                                 1,           // Number of parameters
                                 NULL,        // Parameter types
                                 paramValues, // Parameter values
                                 NULL,        // Parameter lengths
                                 NULL,        // Parameter formats
                                 0);          // Result format (0 = text)

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "dbGetUserIdByUsername failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 0)
    {
        // User not found
        PQclear(res);
        return -1;
    }

    int user_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return user_id;
}

// Add a friend directly (creates mutual friendship)
int dbAddFriend(PGconn* conn, int user_id, const char* friend_username)
{
    // First, get the friend's user_id
    int friend_id = dbGetUserIdByUsername(conn, friend_username);
    if (friend_id < 0)
    {
        return -1; // User not found
    }

    if (friend_id == user_id)
    {
        return -2; // Cannot add yourself as friend
    }

    // Check if friendship already exists
    const char* check_query = "SELECT 1 FROM friend WHERE (u1 = $1 AND u2 = $2) OR (u1 = $2 AND u2 = $1) LIMIT 1";
    char user_id_str[16];
    char friend_id_str[16];
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
    snprintf(friend_id_str, sizeof(friend_id_str), "%d", friend_id);
    
    const char* check_params[] = {user_id_str, friend_id_str};
    PGresult* check_res = PQexecParams(conn, check_query, 2, NULL, check_params, NULL, NULL, 0);

    if (PQresultStatus(check_res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "dbAddFriend check failed: %s\n", PQerrorMessage(conn));
        PQclear(check_res);
        return DB_ERROR;
    }

    if (PQntuples(check_res) > 0)
    {
        // Friendship already exists
        PQclear(check_res);
        return -3;
    }
    PQclear(check_res);

    // Insert friendship (bidirectional)
    const char* insert_query = "INSERT INTO friend (u1, u2) VALUES ($1, $2), ($2, $1)";
    const char* insert_params[] = {user_id_str, friend_id_str};
    
    PGresult* res = PQexecParams(conn, insert_query, 2, NULL, insert_params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "dbAddFriend insert failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }

    PQclear(res);
    return DB_OK;
}

// Send a friend invite
int dbSendFriendInvite(PGconn* conn, int from_user_id, const char* to_username)
{
    // First, get the recipient's user_id
    int to_user_id = dbGetUserIdByUsername(conn, to_username);
    if (to_user_id < 0)
    {
        return -1; // User not found
    }

    if (to_user_id == from_user_id)
    {
        return -2; // Cannot invite yourself
    }

    // Check if they're already friends
    char from_id_str[16];
    char to_id_str[16];
    snprintf(from_id_str, sizeof(from_id_str), "%d", from_user_id);
    snprintf(to_id_str, sizeof(to_id_str), "%d", to_user_id);

    const char* friend_check = "SELECT 1 FROM friend WHERE (u1 = $1 AND u2 = $2) OR (u1 = $2 AND u2 = $1) LIMIT 1";
    const char* friend_params[] = {from_id_str, to_id_str};
    PGresult* friend_res = PQexecParams(conn, friend_check, 2, NULL, friend_params, NULL, NULL, 0);

    if (PQresultStatus(friend_res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "dbSendFriendInvite friend check failed: %s\n", PQerrorMessage(conn));
        PQclear(friend_res);
        return DB_ERROR;
    }

    if (PQntuples(friend_res) > 0)
    {
        // Already friends
        PQclear(friend_res);
        return -3;
    }
    PQclear(friend_res);

    // Check if invite already exists
    const char* invite_check = "SELECT 1 FROM friend_invites WHERE from_user_id = $1 AND to_user_id = $2 AND status = 'pending' LIMIT 1";
    PGresult* invite_res = PQexecParams(conn, invite_check, 2, NULL, friend_params, NULL, NULL, 0);

    if (PQresultStatus(invite_res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "dbSendFriendInvite invite check failed: %s\n", PQerrorMessage(conn));
        PQclear(invite_res);
        return DB_ERROR;
    }

    if (PQntuples(invite_res) > 0)
    {
        // Invite already sent
        PQclear(invite_res);
        return -4;
    }
    PQclear(invite_res);

    // Create the invite
    const char* insert_query = "INSERT INTO friend_invites (from_user_id, to_user_id, status) VALUES ($1, $2, 'pending')";
    PGresult* res = PQexecParams(conn, insert_query, 2, NULL, friend_params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "dbSendFriendInvite insert failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }

    PQclear(res);
    return DB_OK;
}

// Accept a friend invite
int dbAcceptFriendInvite(PGconn* conn, int user_id, int invite_id)
{
    // Begin transaction
    PGresult* begin_res = PQexec(conn, "BEGIN");
    if (PQresultStatus(begin_res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "BEGIN failed: %s\n", PQerrorMessage(conn));
        PQclear(begin_res);
        return DB_ERROR;
    }
    PQclear(begin_res);

    // Get invite details and verify it's for this user
    char invite_id_str[16];
    char user_id_str[16];
    snprintf(invite_id_str, sizeof(invite_id_str), "%d", invite_id);
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);

    const char* get_query = "SELECT from_user_id, to_user_id, status FROM friend_invites WHERE invite_id = $1 AND to_user_id = $2";
    const char* get_params[] = {invite_id_str, user_id_str};
    PGresult* get_res = PQexecParams(conn, get_query, 2, NULL, get_params, NULL, NULL, 0);

    if (PQresultStatus(get_res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "dbAcceptFriendInvite get failed: %s\n", PQerrorMessage(conn));
        PQclear(get_res);
        PQexec(conn, "ROLLBACK");
        return DB_ERROR;
    }

    if (PQntuples(get_res) == 0)
    {
        // Invite not found or not for this user
        PQclear(get_res);
        PQexec(conn, "ROLLBACK");
        return -1;
    }

    const char* status = PQgetvalue(get_res, 0, 2);
    if (strcmp(status, "pending") != 0)
    {
        // Invite already processed
        PQclear(get_res);
        PQexec(conn, "ROLLBACK");
        return -2;
    }

    int from_user_id = atoi(PQgetvalue(get_res, 0, 0));
    char from_user_id_str[16];
    snprintf(from_user_id_str, sizeof(from_user_id_str), "%d", from_user_id);
    PQclear(get_res);

    // Update invite status
    const char* update_query = "UPDATE friend_invites SET status = 'accepted' WHERE invite_id = $1";
    const char* update_params[] = {invite_id_str};
    PGresult* update_res = PQexecParams(conn, update_query, 1, NULL, update_params, NULL, NULL, 0);

    if (PQresultStatus(update_res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "dbAcceptFriendInvite update failed: %s\n", PQerrorMessage(conn));
        PQclear(update_res);
        PQexec(conn, "ROLLBACK");
        return DB_ERROR;
    }
    PQclear(update_res);

    // Add friendship (bidirectional)
    const char* insert_query = "INSERT INTO friend (u1, u2) VALUES ($1, $2), ($2, $1)";
    const char* insert_params[] = {user_id_str, from_user_id_str};
    PGresult* insert_res = PQexecParams(conn, insert_query, 2, NULL, insert_params, NULL, NULL, 0);

    if (PQresultStatus(insert_res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "dbAcceptFriendInvite insert failed: %s\n", PQerrorMessage(conn));
        PQclear(insert_res);
        PQexec(conn, "ROLLBACK");
        return DB_ERROR;
    }
    PQclear(insert_res);

    // Commit transaction
    PGresult* commit_res = PQexec(conn, "COMMIT");
    if (PQresultStatus(commit_res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "COMMIT failed: %s\n", PQerrorMessage(conn));
        PQclear(commit_res);
        return DB_ERROR;
    }
    PQclear(commit_res);

    return DB_OK;
}

// Reject a friend invite
int dbRejectFriendInvite(PGconn* conn, int user_id, int invite_id)
{
    char invite_id_str[16];
    char user_id_str[16];
    snprintf(invite_id_str, sizeof(invite_id_str), "%d", invite_id);
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);

    // Verify invite exists and is for this user
    const char* check_query = "SELECT status FROM friend_invites WHERE invite_id = $1 AND to_user_id = $2";
    const char* check_params[] = {invite_id_str, user_id_str};
    PGresult* check_res = PQexecParams(conn, check_query, 2, NULL, check_params, NULL, NULL, 0);

    if (PQresultStatus(check_res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "dbRejectFriendInvite check failed: %s\n", PQerrorMessage(conn));
        PQclear(check_res);
        return DB_ERROR;
    }

    if (PQntuples(check_res) == 0)
    {
        // Invite not found or not for this user
        PQclear(check_res);
        return -1;
    }

    const char* status = PQgetvalue(check_res, 0, 0);
    if (strcmp(status, "pending") != 0)
    {
        // Invite already processed
        PQclear(check_res);
        return -2;
    }
    PQclear(check_res);

    // Update invite status
    const char* update_query = "UPDATE friend_invites SET status = 'rejected' WHERE invite_id = $1";
    const char* update_params[] = {invite_id_str};
    PGresult* res = PQexecParams(conn, update_query, 1, NULL, update_params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "dbRejectFriendInvite update failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }

    PQclear(res);
    return DB_OK;
}

// Get pending invites for a user
dbInviteList* dbGetPendingInvites(PGconn* conn, int user_id)
{
    char user_id_str[16];
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);

    const char* query = 
        "SELECT fi.invite_id, fi.from_user_id, u.username, fi.status, fi.created_at "
        "FROM friend_invites fi "
        "JOIN \"User\" u ON fi.from_user_id = u.user_id "
        "WHERE fi.to_user_id = $1 AND fi.status = 'pending' "
        "ORDER BY fi.created_at DESC";
    
    const char* params[] = {user_id_str};
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "dbGetPendingInvites failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    int numRow = PQntuples(res);
    dbInviteList* invite_list = malloc(sizeof(dbInviteList));
    invite_list->invites = malloc(numRow * sizeof(dbInvite));
    invite_list->num = numRow;

    for (int i = 0; i < numRow; i++)
    {
        invite_list->invites[i].invite_id = atoi(PQgetvalue(res, i, 0));
        invite_list->invites[i].from_user_id = atoi(PQgetvalue(res, i, 1));
        strncpy(invite_list->invites[i].from_username, PQgetvalue(res, i, 2), 
                sizeof(invite_list->invites[i].from_username) - 1);
        invite_list->invites[i].from_username[31] = '\0';
        
        invite_list->invites[i].to_user_id = user_id;
        
        strncpy(invite_list->invites[i].status, PQgetvalue(res, i, 3), 
                sizeof(invite_list->invites[i].status) - 1);
        invite_list->invites[i].status[15] = '\0';
        
        strncpy(invite_list->invites[i].created_at, PQgetvalue(res, i, 4), 
                sizeof(invite_list->invites[i].created_at) - 1);
        invite_list->invites[i].created_at[31] = '\0';
    }

    PQclear(res);
    return invite_list;
}
