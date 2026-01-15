#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/db/include/db.h"

int main() {
    printf("=== Testing Friend Invite Rejection Fix ===\n\n");
    
    // Connect to database
    PGconn* conn = PQconnectdb("host=localhost port=5433 user=postgres password=postgres dbname=cardio");
    
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        printf("✗ Failed to connect to database: %s\n", PQerrorMessage(conn));
        return 1;
    }
    printf("✓ Connected to database\n");
    
    // Clean up any existing test data
    PQexec(conn, "DELETE FROM friend_invites WHERE from_user_id = 1 AND to_user_id = 2");
    
    printf("\n--- Step 1: Send initial invite from user1 to user2 ---\n");
    int result = dbSendFriendInvite(conn, 1, "user2");
    if (result != DB_OK) {
        printf("✗ Initial invite failed (result=%d)\n", result);
        PQfinish(conn);
        return 1;
    }
    printf("✓ Initial invite sent successfully\n");
    
    printf("\n--- Step 2: Simulate rejection by updating status ---\n");
    PGresult* res = PQexec(conn, "UPDATE friend_invites SET status = 'rejected' WHERE from_user_id = 1 AND to_user_id = 2");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("✗ Failed to update invite status: %s\n", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return 1;
    }
    PQclear(res);
    printf("✓ Invite marked as rejected\n");
    
    printf("\n--- Step 3: Try to send invite again (this should work now) ---\n");
    result = dbSendFriendInvite(conn, 1, "user2");
    if (result != DB_OK) {
        printf("✗ Second invite failed (result=%d) - BUG NOT FIXED!\n", result);
        printf("Error details: The function should update rejected invites to pending\n");
        PQfinish(conn);
        return 1;
    }
    printf("✓ Second invite sent successfully - BUG FIXED!\n");
    
    // Verify the invite status was updated to pending
    printf("\n--- Step 4: Verify invite status is pending ---\n");
    res = PQexec(conn, "SELECT status FROM friend_invites WHERE from_user_id = 1 AND to_user_id = 2");
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        printf("✗ Failed to check invite status\n");
        PQclear(res);
        PQfinish(conn);
        return 1;
    }
    
    const char* status = PQgetvalue(res, 0, 0);
    if (strcmp(status, "pending") == 0) {
        printf("✓ Invite status correctly updated to 'pending'\n");
    } else {
        printf("✗ Invite status is '%s', expected 'pending'\n", status);
        PQclear(res);
        PQfinish(conn);
        return 1;
    }
    PQclear(res);
    
    // Cleanup
    PQexec(conn, "DELETE FROM friend_invites WHERE from_user_id = 1 AND to_user_id = 2");
    PQfinish(conn);
    
    printf("\n=== Test Completed Successfully ===\n");
    printf("The duplicate key constraint bug has been fixed!\n");
    printf("- Previous rejected invites are now properly updated to pending\n");
    printf("- No more duplicate key constraint violations\n");
    
    return 0;
}