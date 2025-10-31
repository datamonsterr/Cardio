/*
 * Password Migration Tool
 *
 * This program migrates existing plaintext passwords in the database
 * to hashed passwords using SHA-512 with salt.
 *
 * Compile: gcc -o migrate_passwords migrate_passwords.c -I/usr/include/postgresql -lpq -lcrypt
 * Run: ./migrate_passwords
 */

#include <crypt.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define conninfo "dbname=cardio user=postgres password=1234 host=localhost port=5433"

// Generate a salt for password hashing using SHA-512
char* generate_salt()
{
    static const char* const saltchars = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    char* salt = malloc(32);
    if (salt == NULL)
    {
        return NULL;
    }

    strcpy(salt, "$6$");

    // Generate random salt characters
    unsigned int seed = time(NULL) ^ getpid() ^ rand();
    for (int i = 0; i < 16; i++)
    {
        salt[3 + i] = saltchars[rand_r(&seed) % 64];
    }
    salt[19] = '$';
    salt[20] = '\0';

    return salt;
}

// Hash a password using SHA-512 with the provided salt
char* hash_password(const char* password, const char* salt)
{
    struct crypt_data data;
    data.initialized = 0;

    char* hash = crypt_r(password, salt, &data);
    if (hash == NULL)
    {
        fprintf(stderr, "Password hashing failed\n");
        return NULL;
    }

    char* result = malloc(strlen(hash) + 1);
    if (result == NULL)
    {
        return NULL;
    }
    strcpy(result, hash);

    return result;
}

// Check if password is already hashed (starts with $6$)
int is_hashed(const char* password)
{
    return (strlen(password) > 3 && password[0] == '$' && password[1] == '6' && password[2] == '$');
}

int main(int argc, char* argv[])
{
    printf("Password Migration Tool\n");
    printf("========================\n\n");

    // Allow custom connection string
    const char* db_conninfo = conninfo;
    if (argc > 1)
    {
        db_conninfo = argv[1];
    }

    printf("Connecting to database...\n");
    PGconn* conn = PQconnectdb(db_conninfo);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    printf("Connected successfully!\n\n");

    // Get all users with plaintext passwords
    const char* query = "SELECT user_id, password FROM \"User\" ORDER BY user_id";
    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return 1;
    }

    int num_users = PQntuples(res);
    printf("Found %d users to process\n\n", num_users);

    int migrated = 0;
    int skipped = 0;
    int failed = 0;

    for (int i = 0; i < num_users; i++)
    {
        int user_id = atoi(PQgetvalue(res, i, 0));
        char* password = PQgetvalue(res, i, 1);

        // Skip if already hashed
        if (is_hashed(password))
        {
            printf("User %d: Already hashed, skipping\n", user_id);
            skipped++;
            continue;
        }

        // Generate salt and hash the password
        char* salt = generate_salt();
        if (salt == NULL)
        {
            fprintf(stderr, "User %d: Failed to generate salt\n", user_id);
            failed++;
            continue;
        }

        char* hashed = hash_password(password, salt);
        free(salt);

        if (hashed == NULL)
        {
            fprintf(stderr, "User %d: Failed to hash password\n", user_id);
            failed++;
            continue;
        }

        // Update the database with hashed password
        const char* update_query = "UPDATE \"User\" SET password = $1 WHERE user_id = $2";
        const char* paramValues[2] = {hashed, PQgetvalue(res, i, 0)};

        PGresult* update_res = PQexecParams(conn, update_query, 2, NULL, paramValues, NULL, NULL, 0);

        if (PQresultStatus(update_res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "User %d: Failed to update password: %s\n", user_id, PQerrorMessage(conn));
            failed++;
        }
        else
        {
            printf("User %d: Password hashed successfully\n", user_id);
            migrated++;
        }

        PQclear(update_res);
        free(hashed);
    }

    PQclear(res);
    PQfinish(conn);

    printf("\n========================\n");
    printf("Migration Summary:\n");
    printf("  Migrated: %d\n", migrated);
    printf("  Skipped:  %d (already hashed)\n", skipped);
    printf("  Failed:   %d\n", failed);
    printf("========================\n");

    return (failed > 0) ? 1 : 0;
}
