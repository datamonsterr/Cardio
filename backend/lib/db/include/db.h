#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include <ctype.h>
#include <stdbool.h>
// #include <msgpack.h>

#define conninfo "dbname=cardio user=postgres password=1234 host=localhost port=5433"
#define REGISTER_OK 0
#define USERNAME_USED 1
#define INVALID_USERNAME 2
#define INVALID_PASSWORD 3
#define SERVER_ERROR -1
#define LOGIN_OK 0
#define NO_USER_FOUND 1
#define SERVER_ERROR -1

struct Player
{
    int player_id;
    char *fullname;
    char gender;
    char *date_of_birth;
    int age;
    char *country;
    char *password;
    unsigned char *avatar;
    float balance;
    int rank;
    char *registration_date;
};

struct Ranking
{
    int balance;
    int player_id;
};

struct Friend
{
    int friendship_id;
    int player1_id;
    int player2_id;
    char *friend_time;
};

struct Room
{
    int room_id;
    int min_player;
    int max_layer;
    int host_id;
    char *status;
};

struct Match
{
    int match_id;
    int room_id;
    int winner;
    char *status;
    char *match_date;
    char *start_time;
    char *end_time;
    unsigned char *progress;
};

struct Host
{
    int host_id;
};

struct Seat
{
    int seat_number;
    int player_id;
    int room_id;
    char *sit;
};

struct History
{
    int match_id;
};

// This function connect to database.
// Output is none if connection is ok, or an error message if connection is failed.
void connection(PGconn *conn);

// This function return a struct of player if success, or an error message if failed.
struct Player getPlayerInfo(PGconn *conn, int player_id);
// This function create a player in database or return an error message if create failed.
void createPlayer(PGconn *conn, int player_id, char *fullname, char gender, char *date_of_birth, int age, char *country, char *password, float balance, int rank, char *registration_date);
// This function delete a player from database or return an error message if delete failed.
void deletePlayer(PGconn *conn, int player_id);
// This function return the leaderboard or an error message if failed.
struct Ranking *getRankingInfo(PGconn *conn);

bool validate_username(char *username);
bool validate_password(char *password);
// This function take input: username, password.
// If username and password are valid, it create a new player in database
// new player_id = number of existed player + 1
int signup(PGconn *conn, char *username, char *password);
// This function take input: username, password
// If username and password are valid and exist in database, return login successful
int login(PGconn *conn, char *username, char *password);