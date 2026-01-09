#include "../include/db.h"
#include "../../logger/include/logger.h"

#define DB_LOG "server.log"

bool validate_username(char* username)
{
    if (strlen(username) < 5)
    {
        return false;
    }

    // alphanumeric characters and underscores only
    for (int i = 0; i < strlen(username); i++)
    {
        if (!isalnum(username[i]) && username[i] != '_')
        {
            return false;
        }
    }

    return true;
}

bool validate_password(char* password)
{
    // if contains at least 10 characters, both numeric and alphabetic
    if (strlen(password) < 10)
    {
        return false;
    }

    bool digit = false;
    bool alpha = false;
    for (int i = 0; i < strlen(password); i++)
    {
        if (isalpha(password[i]))
        {
            alpha = true;
        }
        if (isdigit(password[i]))
        {
            digit = true;
        }
    }

    return alpha && digit;
}

int dbSignup(PGconn* conn, struct dbUser* user)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Starting signup for user='%s'", user->username);
    logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);
    
    if (!validate_username(user->username))
    {
        snprintf(log_msg, sizeof(log_msg), "Invalid username '%s'", user->username);
        logger_ex(DB_LOG, "ERROR", __func__, log_msg, 1);
        return DB_ERROR;
    }
    logger_ex(DB_LOG, "DEBUG", __func__, "Username validation passed", 1);

    if (!validate_password(user->password))
    {
        snprintf(log_msg, sizeof(log_msg), "Invalid password (length=%zu)", strlen(user->password));
        logger_ex(DB_LOG, "ERROR", __func__, log_msg, 1);
        return DB_ERROR;
    }
    logger_ex(DB_LOG, "DEBUG", __func__, "Password validation passed", 1);

    if (strlen(user->email) == 0 || strlen(user->phone) == 0)
    {
        logger_ex(DB_LOG, "ERROR", __func__, "Email or phone is empty", 1);
        return DB_ERROR;
    }
    logger_ex(DB_LOG, "DEBUG", __func__, "Email/phone validation passed", 1);

    char query[256];
    snprintf(query, sizeof(query),
             "select user_id from \"User\" where email = '%s' OR phone = '%s' OR username = '%s';", user->email,
             user->phone, user->username);

    logger_ex(DB_LOG, "DEBUG", __func__, "Checking for existing user...", 1);
    PGresult* res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        snprintf(log_msg, sizeof(log_msg), "PostgreSQL error: %s", PQerrorMessage(conn));
        logger_ex(DB_LOG, "ERROR", __func__, log_msg, 1);
        PQclear(res);
        return DB_ERROR;
    }

    if (PQntuples(res) > 0)
    {
        logger_ex(DB_LOG, "WARN", __func__, "User already exists (email/phone/username conflict)", 1);
        PQclear(res);
        return DB_ERROR;
    }
    logger_ex(DB_LOG, "DEBUG", __func__, "No existing user found, proceeding...", 1);

    PQclear(res);

    // Hash the password before creating the user
    logger_ex(DB_LOG, "DEBUG", __func__, "Generating salt...", 1);
    char* salt = generate_salt();
    if (salt == NULL)
    {
        logger_ex(DB_LOG, "ERROR", __func__, "Failed to generate salt", 1);
        return DB_ERROR;
    }
    logger_ex(DB_LOG, "DEBUG", __func__, "Salt generated successfully", 1);

    logger_ex(DB_LOG, "DEBUG", __func__, "Hashing password...", 1);
    char* hashed_password = hash_password(user->password, salt);
    free(salt);

    if (hashed_password == NULL)
    {
        logger_ex(DB_LOG, "ERROR", __func__, "Failed to hash password", 1);
        return DB_ERROR;
    }
    snprintf(log_msg, sizeof(log_msg), "Password hashed successfully (hash_len=%zu)", strlen(hashed_password));
    logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);

    // Store the hashed password in the user struct
    strncpy(user->password, hashed_password, sizeof(user->password) - 1);
    user->password[sizeof(user->password) - 1] = '\0';
    free(hashed_password);

    logger_ex(DB_LOG, "DEBUG", __func__, "Calling dbCreateUser...", 1);
    int result = dbCreateUser(conn, user);
    snprintf(log_msg, sizeof(log_msg), "dbCreateUser returned %d", result);
    logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);
    return result;
}
