#include "testing.h"
#include "db.h"

TEST(test_db_get_user_info)
{
    PGconn *conn = PQconnectdb(conninfo);

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
    PGconn *conn = PQconnectdb(conninfo);

    struct dbUser user;
    strcpy(user.username, "tester");
    strcpy(user.fullname, "User Test 1");
    strcpy(user.email, "test1@gmail.com");
    strcpy(user.password, "password12345");
    strcpy(user.country, "Vietnam");
    strcpy(user.gender, "Male");
    strcpy(user.phone, "0123456789");
    strcpy(user.dob, "2000-01-01");

    // int signup_result = dbSignup(conn, &user);

    // ASSERT(signup_result == DB_OK);

    // Check if the user is created
    int user_id = dbLogin(conn, "tester", "password12345");
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

int main()
{
    RUN_TEST(test_db_get_user_info);
    RUN_TEST(test_db_signup);
}