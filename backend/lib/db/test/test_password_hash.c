#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "db.h"

void test_generate_salt() {
    printf("Test 1: Generate Salt\n");
    
    char *salt1 = generate_salt();
    char *salt2 = generate_salt();
    
    assert(salt1 != NULL);
    assert(salt2 != NULL);
    
    // Check format: $6$....$
    assert(strncmp(salt1, "$6$", 3) == 0);
    assert(strncmp(salt2, "$6$", 3) == 0);
    
    // Salts should be different
    assert(strcmp(salt1, salt2) != 0);
    
    printf("  ✓ Salt generation works\n");
    printf("  ✓ Salt format: %s\n", salt1);
    
    free(salt1);
    free(salt2);
}

void test_hash_password() {
    printf("\nTest 2: Hash Password\n");
    
    const char *password = "mySecurePass123";
    char *salt = generate_salt();
    
    char *hash1 = hash_password(password, salt);
    char *hash2 = hash_password(password, salt);
    
    assert(hash1 != NULL);
    assert(hash2 != NULL);
    
    // Same password + salt should produce same hash
    assert(strcmp(hash1, hash2) == 0);
    
    // Hash should be longer than password
    assert(strlen(hash1) > strlen(password));
    
    // Hash should start with salt
    assert(strncmp(hash1, salt, strlen(salt)) == 0);
    
    printf("  ✓ Password hashing works\n");
    printf("  ✓ Hash length: %zu\n", strlen(hash1));
    printf("  ✓ Hash: %s\n", hash1);
    
    free(salt);
    free(hash1);
    free(hash2);
}

void test_verify_password() {
    printf("\nTest 3: Verify Password\n");
    
    const char *password = "password12345";
    const char *wrong_password = "wrongpassword";
    
    char *salt = generate_salt();
    char *hash = hash_password(password, salt);
    
    // Correct password should verify
    assert(verify_password(password, hash) == true);
    printf("  ✓ Correct password verified\n");
    
    // Wrong password should not verify
    assert(verify_password(wrong_password, hash) == false);
    printf("  ✓ Wrong password rejected\n");
    
    free(salt);
    free(hash);
}

void test_different_passwords() {
    printf("\nTest 4: Different Passwords\n");
    
    const char *password1 = "password123";
    const char *password2 = "differentpass456";
    
    char *salt1 = generate_salt();
    char *salt2 = generate_salt();
    
    char *hash1 = hash_password(password1, salt1);
    char *hash2 = hash_password(password2, salt2);
    
    // Different passwords should produce different hashes
    assert(strcmp(hash1, hash2) != 0);
    
    printf("  ✓ Different passwords produce different hashes\n");
    
    free(salt1);
    free(salt2);
    free(hash1);
    free(hash2);
}

void test_hash_length() {
    printf("\nTest 5: Hash Length Compatibility\n");
    
    const char *password = "testPassword123456789";
    char *salt = generate_salt();
    char *hash = hash_password(password, salt);
    
    // Hash should fit in 128 char buffer
    assert(strlen(hash) < 128);
    
    printf("  ✓ Hash fits in 128 char buffer (length: %zu)\n", strlen(hash));
    
    free(salt);
    free(hash);
}

int main() {
    printf("===========================================\n");
    printf("  Password Hashing Unit Tests\n");
    printf("===========================================\n\n");
    
    test_generate_salt();
    test_hash_password();
    test_verify_password();
    test_different_passwords();
    test_hash_length();
    
    printf("\n===========================================\n");
    printf("  All Tests Passed! ✓\n");
    printf("===========================================\n");
    
    return 0;
}
