#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include <ctype.h>
#include <stdbool.h>
// #include <msgpack.h> 

#define conninfo "dbname=cardio user=vietanh password=vietzanh204 host=localhost port=5432"
#define REGISTER_OK 0
#define USERNAME_USED 1
#define INVALID_USERNAME 2
#define INVALID_PASSWORD 3
#define SERVER_ERROR -1
#define LOGIN_OK 0
#define NO_USER_FOUND 1
#define SERVER_ERROR -1

struct Player {
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

struct Ranking {
    int balance;
    int player_id;
};

struct Friend {
    int friendship_id;
    int player1_id;
    int player2_id;
    char *friend_time;
};

struct Room {
    int room_id;
    int min_player;
    int max_layer;
    int host_id;
    char *status;
};

struct Match {
    int match_id;
    int room_id;
    int winner;
    char *status;
    char *match_date;
    char *start_time;
    char *end_time;
    unsigned char *progress;
};

struct Host {
    int host_id;
};

struct Seat {
    int seat_number;
    int player_id;
    int room_id;
    char *sit;
};

struct History {
    int match_id;
};


struct Player getPlayerInfo(PGconn *conn, int player_id);
void createPlayer(PGconn *conn, int player_id, char *fullname, char gender, char *date_of_birth, int age, char *country, char *password, float balance, int rank, char *registration_date);
void deletePlayer(PGconn *conn, int player_id);
struct Ranking* getRankingInfo(PGconn *conn);

bool validate_username(char *username); 
bool validate_password(char *password);
int signup(PGconn *conn, char *username, char *password);
int login(PGconn *conn, char *username, char *password);



// "conn" is static global variable
// getPlayerInfo:

