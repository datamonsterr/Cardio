#include "../include/db.h"

int dbLogin(PGconn* conn, char* username, char* password)
{
    char query[256];
    snprintf(query, sizeof(query), "select user_id, password from \"User\" where username = '%s' limit 1;", username);

    PGresult* res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "PostgreSQL error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }

    if (PQntuples(res) == 0)
    {
        PQclear(res);
        return DB_ERROR;
    }

    int user_id = atoi(PQgetvalue(res, 0, 0));
    char* db_password = PQgetvalue(res, 0, 1);

    // Verify the password against the stored hash
    if (verify_password(password, db_password))
    {
        PQclear(res);
        return user_id;
    }

    PQclear(res);
    return DB_ERROR;
}