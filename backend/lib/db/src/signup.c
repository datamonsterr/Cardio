#include "../include/db.h"

bool validate_username(char *username)
{
    // if contains at least 10 characters, only alphabetic
    if (strlen(username) < 5)
    {
        return false;
    }

    for (int i = 0; i < strlen(username); i++)
    {
        if (!isalpha(username[i]))
        {
            return false;
        }
    }

    return true;
}

bool validate_password(char *password)
{
    // if contains at least 10 characters, both numeric and alphabetic
    if (strlen(password) < 10)
    {
        return false;
    }

    bool digit;
    bool alpha;
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

int dbSignup(PGconn *conn, char *username, char *password)
{
    if (!validate_username(username))
    {
        return INVALID_USERNAME;
    }

    if (!validate_password(password))
    {
        return INVALID_PASSWORD;
    }

    char query[256];
    snprintf(query, sizeof(query), "select player_id from player where fullname = '%s';", username);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "PostgreSQL error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return SERVER_ERROR;
    }

    if (PQntuples(res) > 0)
    {
        PQclear(res);
        return USERNAME_USED;
    }

    PQclear(res);
    snprintf(query, sizeof(query), "select count(player_id) + 1 as new_playerID  from player;");
    res = PQexec(conn, query);
    int new_playerID;
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        if (PQntuples(res) == 1)
        {
            new_playerID = atoi(PQgetvalue(res, 0, 0));
            dbCreateUser(conn, new_playerID, username, 'M', "2004-04-09", 20, "Vietnam", password, -1.0, -1, "2024-04-11");
            PQclear(res);
            return REGISTER_OK;
        }
        else
        {
            PQclear(res);
            return SERVER_ERROR;
        }
    }

    // snprintf(query, sizeof(query), "insert into player (player_id, fullname, gender, date_of_birth, age, country, password, avatar, balance, rank, registration_date) values (%d,'%s','',null,20,null,'%s',null,-1.0,-1,null);", new_playerID, username, password);
    // res = PQexec(conn, query);
    // if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    //     fprintf(stderr, "PstgreSQL error: %s\n", PQerrorMessage(conn));
    //     PQclear(res);
    //     return SERVER_ERROR;
    // }

    PQclear(res);
    return REGISTER_OK;
}
