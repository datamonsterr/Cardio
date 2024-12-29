#include "../include/db.h"

// input is player's information
// void function so no output, used to create player and insert player in ranking board
void dbCreateUser(PGconn *conn, int player_id, char *fullname, char gender, char *date_of_birth, int age, char *country, char *password, float balance, int rank, char *registration_date)
{
    char query[256];
    snprintf(query, sizeof(query), "INSERT INTO player (player_id, fullname, gender, date_of_birth, age, country, password, avatar, balance, rank, registration_date) "
                                   "VALUES (%d, '%s', '%c', '%s', %d, '%s', '%s', NULL, %f, %d, '%s')",
             player_id, fullname, gender, date_of_birth, age, country, password, balance, rank, registration_date);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "\nCreating player failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    PQclear(res);
    snprintf(query, sizeof(query), "INSERT INTO ranking (balance, player_id) "
                                   "VALUES (%f, %d)",
             balance, player_id);
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "\nCreating player failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    printf("\nPlayer created successfully.\n");
    PQclear(res);
}

// input is 'conn' - the connection pointer, and player_id;
// output is information of player in form of struct dbUser
struct dbUser dbGetUserInfo(PGconn *conn, int player_id)
{
    char query[256];
    struct dbUser player;
    snprintf(query, sizeof(query), "SELECT * FROM player WHERE player_id = %d", player_id);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "No data retrieved: %s\n", PQerrorMessage(conn));
        PQclear(res);

        player.player_id = -1;
        return player;
    }

    player.player_id = atoi(PQgetvalue(res, 0, 0));
    player.fullname = malloc(strlen(PQgetvalue(res, 0, 1)) + 1);
    strncpy(player.fullname, PQgetvalue(res, 0, 1), sizeof(player.fullname) - 1);
    player.gender = PQgetvalue(res, 0, 2)[0];
    player.date_of_birth = malloc(strlen(PQgetvalue(res, 0, 3)) + 1);
    strncpy(player.date_of_birth, PQgetvalue(res, 0, 3), sizeof(player.date_of_birth) - 1);
    player.age = atoi(PQgetvalue(res, 0, 4));
    player.country = malloc(strlen(PQgetvalue(res, 0, 5)) + 1);
    strncpy(player.country, PQgetvalue(res, 0, 5), sizeof(player.country) - 1);
    player.password = malloc(strlen(PQgetvalue(res, 0, 6)) + 1);
    strncpy(player.password, PQgetvalue(res, 0, 6), sizeof(player.password) - 1);

    int avatar_length = PQgetlength(res, 0, 7);
    player.avatar = malloc(avatar_length);
    if (player.avatar != NULL)
    {
        memcpy(player.avatar, PQgetvalue(res, 0, 7), avatar_length); // copy a specific number of bytes
    }

    player.balance = atof(PQgetvalue(res, 0, 8));
    player.rank = atoi(PQgetvalue(res, 0, 9));
    player.registration_date = malloc(strlen(PQgetvalue(res, 0, 10)) + 1);
    strncpy(player.registration_date, PQgetvalue(res, 0, 10), sizeof(player.registration_date) - 1);

    PQclear(res);
    return player;
}

// input is player_id
// void function so no output, used to delete player
void dbDeleteUser(PGconn *conn, int player_id)
{
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM player WHERE player_id = %d RETURNING *", player_id);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "\nDeleting player failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    printf("%s", PQerrorMessage(conn));
    printf("Player with ID %d deleted successfully.\n", player_id);
    PQclear(res);
}

// no need input
// output is the list of players in ranking board
struct Ranking *dbGetScoreBoard(PGconn *conn)
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
    struct Ranking *leaderboard = malloc(numRow * sizeof(struct Ranking));
    for (int i = 0; i < numRow; i++)
    {
        leaderboard[i].balance = atoi(PQgetvalue(res, i, 0));
        leaderboard[i].player_id = atoi(PQgetvalue(res, i, 1));
    }

    PQclear(res);
    return leaderboard;
}
