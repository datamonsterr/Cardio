# Friend Management Implementation

This document describes the implementation of friend invite/add functionality in the Cardio poker server.

## Overview

The friend management system allows users to:
1. Send friend invites to other users
2. View pending friend invites
3. Accept or reject friend invites
4. Directly add friends (mutual friendship)

## Protocol Specification

### Packet Types

New packet types added to the protocol (range 910-952):

- **PACKET_ADD_FRIEND (910)** - Directly add a friend (mutual)
  - Response codes: 911 (OK), 912 (NOT_OK), 913 (ALREADY_EXISTS)

- **PACKET_INVITE_FRIEND (920)** - Send a friend invite
  - Response codes: 921 (OK), 922 (NOT_OK), 923 (ALREADY_SENT)

- **PACKET_ACCEPT_INVITE (930)** - Accept a friend invite
  - Response codes: 931 (OK), 932 (NOT_OK)

- **PACKET_REJECT_INVITE (940)** - Reject a friend invite
  - Response codes: 941 (OK), 942 (NOT_OK)

- **PACKET_GET_INVITES (950)** - Get pending invites
  - Response codes: 951 (OK), 952 (NOT_OK)

### Message Formats

#### Add Friend Request (C2S)
```json
{
  "username": "<string>"  // Username to add as friend
}
```

#### Invite Friend Request (C2S)
```json
{
  "username": "<string>"  // Username to invite
}
```

#### Accept/Reject Invite Request (C2S)
```json
{
  "invite_id": <int>  // ID of the invite to accept/reject
}
```

#### Get Invites Response (S2C)
```json
[
  {
    "invite_id": <int>,
    "from_user_id": <int>,
    "from_username": "<string>",
    "status": "<string>",  // "pending", "accepted", "rejected"
    "created_at": "<string>"
  },
  ...
]
```

## Database Schema

### friend_invites Table

```sql
CREATE TABLE friend_invites
(
    invite_id SERIAL PRIMARY KEY,
    from_user_id INTEGER NOT NULL,
    to_user_id INTEGER NOT NULL,
    status VARCHAR(16) NOT NULL DEFAULT 'pending' 
           CHECK (status IN ('pending', 'accepted', 'rejected')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (from_user_id) REFERENCES "User"(user_id) ON DELETE CASCADE,
    FOREIGN KEY (to_user_id) REFERENCES "User"(user_id) ON DELETE CASCADE,
    UNIQUE (from_user_id, to_user_id)
);

CREATE INDEX idx_friend_invites_to_user ON friend_invites(to_user_id) 
    WHERE status = 'pending';
CREATE INDEX idx_friend_invites_from_user ON friend_invites(from_user_id);
```

## Implementation Details

### Files Modified/Created

1. **Protocol Definition** ([server/include/protocol.h](server/include/protocol.h))
   - Added packet type constants
   - Added request/response structures
   - Added function declarations

2. **Protocol Encoding/Decoding** ([server/src/protocol.c](server/src/protocol.c))
   - `decode_add_friend_request()` - Decode add friend request
   - `decode_invite_friend_request()` - Decode invite request
   - `decode_invite_action_request()` - Decode accept/reject request
   - `encode_invites_response()` - Encode invite list response

3. **Database Operations** ([server/lib/db/src/friend.c](server/lib/db/src/friend.c))
   - `dbGetUserIdByUsername()` - Get user ID from username
   - `dbAddFriend()` - Add mutual friendship
   - `dbSendFriendInvite()` - Send friend invite
   - `dbAcceptFriendInvite()` - Accept invite and create friendship
   - `dbRejectFriendInvite()` - Reject invite
   - `dbGetPendingInvites()` - Get user's pending invites

4. **Request Handlers** ([server/src/handler.c](server/src/handler.c))
   - `handle_add_friend_request()` - Handle add friend
   - `handle_invite_friend_request()` - Handle send invite
   - `handle_accept_invite_request()` - Handle accept invite
   - `handle_reject_invite_request()` - Handle reject invite
   - `handle_get_invites_request()` - Handle get invites

5. **Request Routing** ([server/src/main.c](server/src/main.c))
   - Added case statements to route new packet types to handlers

6. **Database Schema** ([database/schemas.sql](database/schemas.sql))
   - Added friend_invites table definition

### Error Handling

Each operation validates:
- User is logged in
- Target user exists
- No duplicate invites/friendships
- User owns the invite they're acting on
- Invite is in correct state

Error codes:
- `-1`: Target user not found
- `-2`: Cannot add/invite yourself
- `-3`: Already friends
- `-4`: Invite already sent
- `DB_ERROR`: Database operation failed

### Database Transactions

The `dbAcceptFriendInvite()` function uses a transaction to ensure atomicity:
1. BEGIN transaction
2. Verify invite exists and is pending
3. Update invite status to 'accepted'
4. Insert bidirectional friendship
5. COMMIT transaction

If any step fails, the transaction is rolled back.

## Testing

### End-to-End Test

Located at: [server/test/e2e_friend_test.c](server/test/e2e_friend_test.c)

The test covers:
1. User1 sends invite to User2
2. User2 checks pending invites
3. User2 accepts invite
4. User1 sends invite to User3  
5. User3 rejects invite

### Running the Test

```bash
# Start the server (in one terminal)
cd server/build
./Cardio_server

# Create database table (if not done)
sudo docker-compose exec -T db psql -U postgres -d cardio < database/schemas.sql

# Run the test (in another terminal)
cd server/build
./e2e_friend_test
```

### Test Users

The test assumes these users exist in the database:
- user1 / user1
- user2 / user2
- user3 / user3

## Usage Example

### Client Flow

1. **Login**
   ```
   → PACKET_LOGIN with username/password
   ← LOGIN_RESPONSE with user info
   ```

2. **Send Invite**
   ```
   → PACKET_INVITE_FRIEND with target username
   ← R_INVITE_FRIEND_OK or error code
   ```

3. **Check Invites**
   ```
   → PACKET_GET_INVITES
   ← Array of pending invites
   ```

4. **Accept Invite**
   ```
   → PACKET_ACCEPT_INVITE with invite_id
   ← R_ACCEPT_INVITE_OK or error code
   ```

## Future Improvements

1. **Notifications** - Real-time notification when receiving invite
2. **Friend Status** - Online/offline status tracking
3. **Friend List Pagination** - For users with many friends
4. **Block User** - Ability to block unwanted invites
5. **Friend Removal** - Remove existing friendships
6. **Friend Requests Expiration** - Auto-expire old pending invites

## Security Considerations

1. **Authorization** - All operations verify user is logged in
2. **Input Validation** - Usernames validated against database
3. **Race Conditions** - Unique constraints prevent duplicate invites
4. **SQL Injection** - Parameterized queries used throughout
5. **Privacy** - Users cannot see others' friend lists without permission

## Logging

All friend operations are logged with:
- User who initiated the action
- Target username/invite ID
- Operation result (success/failure)
- Error details on failure

Log levels used:
- **INFO** - Successful operations
- **WARN** - Expected failures (user not found, already friends, etc.)
- **ERROR** - Unexpected failures (database errors, invalid packets)
