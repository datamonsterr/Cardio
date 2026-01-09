#include "../include/db.h"
#include "../../../lib/logger/include/logger.h"
#include <crypt.h>
#include <time.h>
#include <unistd.h>

#define DB_LOG "server.log"

// Generate a salt for password hashing using SHA-512
// Returns a pointer to the salt string (must be freed by caller)
char* generate_salt()
{
    static const char* const saltchars = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    // Format: $6$ indicates SHA-512, followed by up to 16 characters of salt
    char* salt = malloc(32);
    if (salt == NULL)
    {
        return NULL;
    }

    strcpy(salt, "$6$");

    // Generate random salt characters using better seed
    static int initialized = 0;
    if (!initialized)
    {
        srand(time(NULL) ^ getpid());
        initialized = 1;
    }

    // Use rand() but add additional randomness from time for each character
    unsigned int seed = time(NULL) ^ getpid() ^ rand();
    for (int i = 0; i < 16; i++)
    {
        seed = seed * 1103515245 + 12345; // Linear congruential generator
        salt[3 + i] = saltchars[(seed >> 16) % 64];
    }
    salt[19] = '$';
    salt[20] = '\0';

    return salt;
}

// Hash a password using SHA-512 with the provided salt
// Returns the hashed password (caller must free the returned string)
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

    // Allocate and copy the hash
    char* result = malloc(strlen(hash) + 1);
    if (result == NULL)
    {
        return NULL;
    }
    strcpy(result, hash);

    return result;
}

// Verify a password against a stored hash
// Returns true if the password matches, false otherwise
bool verify_password(const char* password, const char* hash)
{
    char log_msg[256];
    struct crypt_data data;
    data.initialized = 0;

    snprintf(log_msg, sizeof(log_msg), "verify_password: password_len=%zu hash_len=%zu hash_prefix='%.20s'", 
             strlen(password), strlen(hash), hash);
    logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);

    char* result = crypt_r(password, hash, &data);
    if (result == NULL)
    {
        logger_ex(DB_LOG, "ERROR", __func__, "crypt_r returned NULL", 1);
        return false;
    }

    snprintf(log_msg, sizeof(log_msg), "verify_password: result_len=%zu result_prefix='%.20s'", 
             strlen(result), result);
    logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);
    
    bool match = strcmp(result, hash) == 0;
    snprintf(log_msg, sizeof(log_msg), "verify_password: match=%s", match ? "TRUE" : "FALSE");
    logger_ex(DB_LOG, "DEBUG", __func__, log_msg, 1);

    return match;
}
