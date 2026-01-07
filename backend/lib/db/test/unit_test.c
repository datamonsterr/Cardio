#include "db.h"
#include "testing.h"

TEST(test_db_get_user_info)
{
    PGconn* conn = PQconnectdb(conninfo);

    int user_id = dbLogin(conn, "user2", "password12345");
    if (user_id == DB_ERROR)
    {
        fprintf(stderr, "Login failed\n");
    }
    else
    {
        printf("Login successful\n");
    }
    struct dbUser user = dbGetUserInfo(conn, user_id);

    ASSERT_STR_EQ(user.username, "user2");
    ASSERT_STR_EQ(user.email, "user2@example.com");

    PQfinish(conn);
}

TEST(test_db_signup)
{
    PGconn* conn = PQconnectdb(conninfo);

    struct dbUser user;
    strcpy(user.username, "tester01abc");
    strcpy(user.fullname, "User Test 1");
    strcpy(user.email, "test1abc@gmail.com");
    strcpy(user.password, "password12345");
    strcpy(user.country, "Vietnam");
    strcpy(user.gender, "Male");
    strcpy(user.phone, "03283617384");
    strcpy(user.dob, "2000-01-01");

    int signup_result = dbSignup(conn, &user);

    ASSERT(signup_result == DB_OK);

    // Check if the user is created
    int user_id = dbLogin(conn, "tester01abc", "password12345");
    if (user_id == DB_ERROR)
    {
        fprintf(stderr, "Login failed\n");
    }
    else
    {
        printf("Login successful\n");
    }

    struct dbUser new_user = dbGetUserInfo(conn, user_id);

    ASSERT_STR_EQ(new_user.username, user.username);
    ASSERT_STR_EQ(new_user.email, user.email);
    ASSERT_STR_EQ(new_user.fullname, user.fullname);
    fprintf(stderr, "%s %s", new_user.fullname, user.fullname);

    PQfinish(conn);
}

TEST(test_db_scoreboard)
{
    PGconn* conn = PQconnectdb(conninfo);
    dbScoreboard* leaderboard = dbGetScoreBoard(conn);

    ASSERT(leaderboard->size > 0);
    ASSERT(leaderboard->players[0].balance == 3550);
    PQfinish(conn);
}

TEST(test_db_friendlist)
{
    PGconn* conn = PQconnectdb(conninfo);
    
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return;
    }
    
    FriendList* friendlist = dbGetFriendList(conn, 1);
    
    if (friendlist == NULL)
    {
        fprintf(stderr, "Failed to get friendlist\n");
        PQfinish(conn);
        return;
    }

    ASSERT(friendlist->num > 0);
    ASSERT(friendlist->friends[0].user_id == 2);
    ASSERT(strcmp(friendlist->friends[0].user_name, "user2") == 0);
    
    free(friendlist);
    PQfinish(conn);
}

int main()
{
    // RUN_TEST(test_db_get_user_info);
    // RUN_TEST(test_db_signup);
    // RUN_TEST(test_db_scoreboard);
    RUN_TEST(test_db_friendlist);
}