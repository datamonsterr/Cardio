#include "../include/db.h"

// input is user's information
// void function so no output, used to create user and insert user in ranking board
void dbCreateUser(PGconn *conn, struct dbUser *u)
{
    char query[256];
    snprintf(query, sizeof(query), "INSERT INTO \"User\" (username, full_name, email, phone, dob, password, country, gender, balance) "
                                   "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 1000)",
             u->username, u->fullname, u->email, u->phone, u->dob, u->password, u->country, u->gender);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "\nCreating user failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    PQclear(res);
    printf("\nuser created successfully.\n");
    PQclear(res);
}

// input is 'conn' - the connection pointer, and user_id;
// output is information of user in form of struct dbUser
struct dbUser dbGetUserInfo(PGconn *conn, int user_id)
{
    char query[256];
    struct dbUser user;
    snprintf(query, sizeof(query), "SELECT username, email, phone, dob, country, gender, balance, registration_date, full_name FROM \"User\" WHERE user_id = %d LIMIT 1", user_id);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "No data retrieved: %s\n", PQerrorMessage(conn));
        PQclear(res);

        user.user_id = -1;
        return user;
    }

    user.user_id = user_id;
    strncpy(user.username, PQgetvalue(res, 0, 0), sizeof(user.username));
    strncpy(user.email, PQgetvalue(res, 0, 1), sizeof(user.email));
    strncpy(user.phone, PQgetvalue(res, 0, 2), sizeof(user.phone));
    strncpy(user.dob, PQgetvalue(res, 0, 3), sizeof(user.dob));
    strncpy(user.country, PQgetvalue(res, 0, 4), sizeof(user.country));
    strncpy(user.gender, PQgetvalue(res, 0, 5), sizeof(user.gender));
    user.balance = atof(PQgetvalue(res, 0, 6));
    strncpy(user.registration_date, PQgetvalue(res, 0, 7), sizeof(user.registration_date));
    strncpy(user.fullname, PQgetvalue(res, 0, 8), sizeof(user.fullname));

    PQclear(res);
    return user;
}

// input is user_id
// void function so no output, used to delete user
void dbDeleteUser(PGconn *conn, int user_id)
{
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM user WHERE user_id = %d RETURNING *", user_id);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "\nDeleting user failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    printf("%s", PQerrorMessage(conn));
    printf("user with ID %d deleted successfully.\n", user_id);
    PQclear(res);
}

// no need input
// output is the list of users in ranking board
struct dbRanking *dbGetScoreBoard(PGconn *conn)
{
    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM ranking ORDER BY balance DESC");

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "\nFailed to get ranking board information: %s\n", PQerrorMessage(conn));
        PQclear(res);
    }

    printf("%d\t%d", atoi(PQgetvalue(res, 0, 0)), atoi(PQgetvalue(res, 0, 1)));

    int numRow = PQntuples(res);
    struct dbRanking *leaderboard = malloc(numRow * sizeof(struct dbRanking));
    for (int i = 0; i < numRow; i++)
    {
        leaderboard[i].balance = atoi(PQgetvalue(res, i, 0));
        leaderboard[i].user_id = atoi(PQgetvalue(res, i, 1));
    }

    PQclear(res);
    return leaderboard;
}