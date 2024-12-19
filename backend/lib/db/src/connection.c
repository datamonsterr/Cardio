#include "../include/db.h"


int main() {
    // char *conninfo = "dbname=cardio user=vietanh password=vietzanh204 host=localhost port=5432";
    PGconn *conn = PQconnectdb(conninfo);

    if (PQstatus(conn)!=CONNECTION_OK) {
        printf("Connection to database fails: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

// -----------------------------------------------------------
    // printf("Connection established\n");
    // printf("Port: %s\n", PQport(conn));
    // printf("Host: %s\n", PQhost(conn));
    // printf("Database name: %s\n", PQdb(conn));

    // createPlayer(conn, 10, "Vanh", 'M', "2004-04-09", 20, "Vietnam", "vietzanh204", 100, 1, "2024-04-11");

    // deletePlayer(conn, 10);

    // createPlayer(conn, 10, "Vanh1", 'M', "2004-04-09", 20, "Vietnam", "vietzanh204", 100, 1, "2024-04-11");
    // createPlayer(conn, 20, "Vanh2", 'M', "2000-04-09", 24, "Vietnam", "vietzanh204", 200, 1, "2024-04-11");

    // struct Ranking* leaderboard = getRankingInfo(conn);
    // for (int i=0;i<sizeof(leaderboard) / sizeof(leaderboard[0].balance);i++) {
    //     printf("%d\t%d\n",leaderboard[i].player_id,leaderboard[i].balance);
    // }
    // -------------------------------------------------------------------------
int choice;
    printf("Signup: 1\nLogin: 2\n");
    scanf("%d",&choice);

    char username[50];
    char password[50];

    if (choice == 1) {
        printf("Enter username: ");
        scanf("%s", username);
        printf("Enter password: ");
        scanf("%s", password);
        
        printf("Signup result: ");
        int signup_result = signup(conn, username, password);
        switch(signup_result) {
            case REGISTER_OK:
                // createPlayer(conn, rand(), username, 'M', "2004-04-09", 20, "Vietnam", password, -1.0, -1, "2024-04-11");
                // createPlayer(conn, 10, "Vanh", 'M', "2004-04-09", 20, "Vietnam", "vietzanh204", 100, 1, "2024-04-11");
                printf("REGISTOR_OK\n");
                break;
            case USERNAME_USED:
                printf("USERNAME_USED\n");
                break;
            case INVALID_USERNAME:
                printf("INVALID_USERNAME\n");
                break;
            case INVALID_PASSWORD:
                printf("INVALID_PASSWORD\n");
                break;
            case SERVER_ERROR:
                printf("SERVER_ERROR\n");
                break;
            default:
                break;
        }
    }

    if (choice == 2) {
        printf("Enter username: ");
        scanf("%s", username);
        printf("Enter password: ");
        scanf("%s", password);

        printf("Login result: ");
        int login_result = login(conn, username, password);
        switch(login_result) {
            case LOGIN_OK:
                printf("LOGIN_OK\n");
                break;
            case NO_USER_FOUND:
                printf("NO_USER_FOUND\n");
                break;
            case SERVER_ERROR:
                printf("SERVER_ERROR\n");
                break;
            default:
                break;
        }
    }


    PQfinish(conn);
    return 0;
}