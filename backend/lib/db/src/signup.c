#include "../include/db.h"

bool validate_username(char *username)
{
    if (strlen(username) < 5)
    {
        return false;
    }

    // alphanumeric characters only
    for (int i = 0; i < strlen(username); i++)
    {
        if (!isalnum(username[i]))
        {
            return false;
        }
    }

    return true;
}

bool validate_password(char *password)
{
    // if contains at least 10 characters, both numeric and alphabetic
    if (strlen(password) < 10)
    {
        return false;
    }

    bool digit;
    bool alpha;
    for (int i = 0; i < strlen(password); i++)
    {
        if (isalpha(password[i]))
        {
            alpha = true;
        }
        if (isdigit(password[i]))
        {
            digit = true;
        }
    }

    return alpha && digit;
}

int dbSignup(PGconn *conn, struct dbUser *user)
{
    if (!validate_username(user->username))
    {
        fprintf(stderr, "dbSignup: Invalid username %s\n", user->username);
        return DB_ERROR;
    }

    if (!validate_password(user->password))
    {
        fprintf(stderr, "dbSignup: Invalid password %s\n", user->password);
        return DB_ERROR;
    }

    if (strlen(user->email) == 0 || strlen(user->phone) == 0)
    {
        fprintf(stderr, "dbSignup: Email or phone is empty\n");
        return DB_ERROR;
    }

    char query[256];
    snprintf(query, sizeof(query), "select user_id from \"User\" where email = '%s' OR phone = '%s' OR username = '%s';", user->email, user->phone, user->username);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "PstgreSQL error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return DB_ERROR;
    }

    if (PQntuples(res) > 0)
    {
        PQclear(res);
        return DB_ERROR;
    }

    PQclear(res);
    
    // Hash the password before creating the user
    char *salt = generate_salt();
    if (salt == NULL) {
        fprintf(stderr, "dbSignup: Failed to generate salt\n");
        return DB_ERROR;
    }
    
    char *hashed_password = hash_password(user->password, salt);
    free(salt);
    
    if (hashed_password == NULL) {
        fprintf(stderr, "dbSignup: Failed to hash password\n");
        return DB_ERROR;
    }
    
    // Store the hashed password in the user struct
    strncpy(user->password, hashed_password, sizeof(user->password) - 1);
    user->password[sizeof(user->password) - 1] = '\0';
    free(hashed_password);
    
    return dbCreateUser(conn, user);
}
