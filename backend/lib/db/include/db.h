#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include <ctype.h>
#include <stdbool.h>
// #include <msgpack.h>

#define conninfo "dbname=cardio user=postgres password=1234 host=localhost port=5433"
#define DB_ERROR -200
#define DB_OK -100

struct dbUser
{
    int user_id;
    int balance;
    char username[32];
    char fullname[64];
    char email[64];
    char password[32];
    char country[32];
    char phone[16];
    char dob[16];
    char registration_date[16];
    char gender[8];
} typedef dbUser;

typedef struct
{
    int balance;
    int user_id;
} dbRanking;

typedef struct
{
    dbRanking *players;
    int size;
} dbScoreboard;

typedef struct
{
    int user_id;
    char user_name[32];
} dbFriend;

typedef struct
{
    dbFriend *friends;
    int num;
} FriendList;

// This function connect to database.
// Output is none if connection is ok, or an error message if connection is failed.
void connection(PGconn *conn);

// This function return a struct of user if success, or an error message if failed.
struct dbUser dbGetUserInfo(PGconn *conn, int user_id);
// This function create a user in database or return an error message if create failed.
int dbCreateUser(PGconn *conn, struct dbUser *user);
// This function delete a user from database or return an error message if delete failed.
void dbDeleteUser(PGconn *conn, int user_id);
// This function return the leaderboard or an error message if failed.
dbScoreboard *dbGetScoreBoard(PGconn *conn);
// This function return the friendlist of a player
FriendList *dbGetFriendList(PGconn *conn, int user_id);

// This function take input: username, password.
// If username and password are valid, it create a new user in database
// new user_id = number of existed user + 1
int dbSignup(PGconn *conn, struct dbUser *user);
// This function take input: username, password
// If username and password are valid and exist in database, return dbLogin successful
int dbLogin(PGconn *conn, char *username, char *password);