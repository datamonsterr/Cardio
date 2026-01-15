#include "../include/db.h"

/**
 * Update user balance in database
 * @param conn Database connection
 * @param user_id User ID to update
 * @param new_balance New balance amount
 * @return DB_OK on success, DB_ERROR on failure
 */
int dbUpdateBalance(PGconn* conn, int user_id, int new_balance)
{
    if (!conn || user_id <= 0) {
        return DB_ERROR;
    }
    
    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE \"User\" SET balance = %d WHERE user_id = %d",
             new_balance, user_id);
    
    PGresult* res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Update balance failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }
    
    // Check if any rows were affected
    char* affected_rows = PQcmdTuples(res);
    int rows_updated = atoi(affected_rows);
    
    PQclear(res);
    
    if (rows_updated == 0) {
        fprintf(stderr, "No user found with ID %d\n", user_id);
        return DB_ERROR;
    }
    
    return DB_OK;
}

/**
 * Add amount to user balance in database
 * @param conn Database connection
 * @param user_id User ID to update
 * @param amount Amount to add (can be negative to subtract)
 * @return DB_OK on success, DB_ERROR on failure
 */
int dbAddToBalance(PGconn* conn, int user_id, int amount)
{
    if (!conn || user_id <= 0) {
        return DB_ERROR;
    }
    
    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE \"User\" SET balance = balance + %d WHERE user_id = %d",
             amount, user_id);
    
    PGresult* res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Add to balance failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }
    
    // Check if any rows were affected
    char* affected_rows = PQcmdTuples(res);
    int rows_updated = atoi(affected_rows);
    
    PQclear(res);
    
    if (rows_updated == 0) {
        fprintf(stderr, "No user found with ID %d\n", user_id);
        return DB_ERROR;
    }
    
    return DB_OK;
}

/**
 * Get current user balance from database
 * @param conn Database connection
 * @param user_id User ID to query
 * @return Balance amount on success, -1 on failure
 */
int dbGetBalance(PGconn* conn, int user_id)
{
    if (!conn || user_id <= 0) {
        return -1;
    }
    
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT balance FROM \"User\" WHERE user_id = %d",
             user_id);
    
    PGresult* res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Get balance query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    
    if (PQntuples(res) == 0) {
        fprintf(stderr, "No user found with ID %d\n", user_id);
        PQclear(res);
        return -1;
    }
    
    int balance = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    
    return balance;
}

/**
 * Atomically transfer balance from one user to another
 * @param conn Database connection
 * @param from_user_id Source user ID
 * @param to_user_id Destination user ID
 * @param amount Amount to transfer
 * @return DB_OK on success, DB_ERROR on failure
 */
int dbTransferBalance(PGconn* conn, int from_user_id, int to_user_id, int amount)
{
    if (!conn || from_user_id <= 0 || to_user_id <= 0 || amount <= 0) {
        return DB_ERROR;
    }
    
    // Start transaction
    PGresult* res = PQexec(conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Failed to start transaction: %s", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }
    PQclear(res);
    
    // Check if source user has sufficient balance
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT balance FROM \"User\" WHERE user_id = %d FOR UPDATE",
             from_user_id);
    
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "Failed to lock source user balance: %s", PQerrorMessage(conn));
        PQclear(res);
        PQexec(conn, "ROLLBACK");
        return DB_ERROR;
    }
    
    int current_balance = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    
    if (current_balance < amount) {
        fprintf(stderr, "Insufficient balance: user %d has %d, needs %d\n", 
                from_user_id, current_balance, amount);
        PQexec(conn, "ROLLBACK");
        return DB_ERROR;
    }
    
    // Subtract from source user
    snprintf(query, sizeof(query),
             "UPDATE \"User\" SET balance = balance - %d WHERE user_id = %d",
             amount, from_user_id);
    
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Failed to subtract from source balance: %s", PQerrorMessage(conn));
        PQclear(res);
        PQexec(conn, "ROLLBACK");
        return DB_ERROR;
    }
    PQclear(res);
    
    // Add to destination user
    snprintf(query, sizeof(query),
             "UPDATE \"User\" SET balance = balance + %d WHERE user_id = %d",
             amount, to_user_id);
    
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Failed to add to destination balance: %s", PQerrorMessage(conn));
        PQclear(res);
        PQexec(conn, "ROLLBACK");
        return DB_ERROR;
    }
    PQclear(res);
    
    // Commit transaction
    res = PQexec(conn, "COMMIT");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Failed to commit transaction: %s", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }
    PQclear(res);
    
    return DB_OK;
}