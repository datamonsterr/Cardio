#include "../include/db.h"
#include "../../logger/include/logger.h"

#define DB_LOG "server.log"

int dbLogin(PGconn* conn, char* username, char* password)
{
    char log_msg[256];
    char query[256];
    snprintf(query, sizeof(query), "select user_id, password from \"User\" where username = '%s' limit 1;", username);

    snprintf(log_msg, sizeof(log_msg), "dbLogin: Querying for user='%s'", username);
    logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);
    
    PGresult* res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        snprintf(log_msg, sizeof(log_msg), "PostgreSQL error: %s", PQerrorMessage(conn));
        logger_ex(DB_LOG, "ERROR", __func__, log_msg, 1);
        PQclear(res);
        return DB_ERROR;
    }

    if (PQntuples(res) == 0)
    {
        snprintf(log_msg, sizeof(log_msg), "User '%s' not found in database", username);
        logger_ex(DB_LOG, "WARN", __func__, log_msg, 1);
        PQclear(res);
        return DB_ERROR;
    }

    int user_id = atoi(PQgetvalue(res, 0, 0));
    char* db_password = PQgetvalue(res, 0, 1);
    
    snprintf(log_msg, sizeof(log_msg), "Found user_id=%d, verifying password (hash_len=%zu)...", user_id, strlen(db_password));
    logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);

    // Verify the password against the stored hash
    if (verify_password(password, db_password))
    {
        snprintf(log_msg, sizeof(log_msg), "Password verification SUCCESS for user='%s' (id=%d)", username, user_id);
        logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);
        PQclear(res);
        return user_id;
    }

    snprintf(log_msg, sizeof(log_msg), "Password verification FAILED for user='%s'", username);
    logger_ex(DB_LOG, "WARN", __func__, log_msg, 1);
    PQclear(res);
    return DB_ERROR;
}