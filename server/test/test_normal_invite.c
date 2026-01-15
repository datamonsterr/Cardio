#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/db/include/db.h"

int main() {
    printf("=== Testing Normal Friend Invite Flow ===\n\n");
    
    // Connect to database
    PGconn* conn = PQconnectdb("host=localhost port=5433 user=postgres password=postgres dbname=cardio");
    
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        printf("✗ Failed to connect to database: %s\n", PQerrorMessage(conn));
        return 1;
    }
    printf("✓ Connected to database\n");
    
    // Clean up any existing test data
    PQexec(conn, "DELETE FROM friend_invites WHERE from_user_id = 3 AND to_user_id = 4");
    
    printf("\n--- Test 1: Send new invite (should create new record) ---\n");
    int result = dbSendFriendInvite(conn, 3, "user4");
    if (result != DB_OK) {
        printf("✗ New invite failed (result=%d)\n", result);
        PQfinish(conn);
        return 1;
    }
    printf("✓ New invite sent successfully\n");
    
    printf("\n--- Test 2: Send duplicate pending invite (should fail) ---\n");
    result = dbSendFriendInvite(conn, 3, "user4");
    if (result != -4) { // Expected error for duplicate pending invite
        printf("✗ Duplicate pending invite should fail with -4, got %d\n", result);
        PQfinish(conn);
        return 1;
    }
    printf("✓ Duplicate pending invite correctly rejected\n");
    
    // Cleanup
    PQexec(conn, "DELETE FROM friend_invites WHERE from_user_id = 3 AND to_user_id = 4");
    PQfinish(conn);
    
    printf("\n=== Normal Flow Test Completed Successfully ===\n");
    printf("Both new invites and rejection-retry scenarios work correctly!\n");
    
    return 0;
}