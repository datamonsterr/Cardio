#include "../include/db.h"

int login(PGconn *conn, char *username, char *password) {
    char query[256];
    snprintf(query, sizeof(query), "select password from player where fullname = '%s';", username);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "PostgreSQL error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return SERVER_ERROR;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return NO_USER_FOUND;
    }

    char *db_password = PQgetvalue(res,0,0);
    if (strcmp(db_password, password) == 0) {
        PQclear(res);
        return LOGIN_OK;
    }

    PQclear(res);
    return NO_USER_FOUND;
}