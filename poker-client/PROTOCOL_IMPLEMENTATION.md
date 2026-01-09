# Poker Client - Protocol Implementation

This document describes the TCP protocol implementation for the poker client.

## Overview

The client now implements a complete TCP communication layer using MessagePack serialization to communicate with the C server backend. The implementation mirrors the server's protocol exactly.

## Architecture

### Protocol Layer (`src/renderer/src/services/protocol/`)

- **constants.ts**: Protocol version, packet types, and response codes
- **types.ts**: TypeScript interfaces matching C structs
- **codec.ts**: Packet encoding/decoding functions using MessagePack
- **index.ts**: Module exports

### Network Layer (`src/renderer/src/services/network/`)

- **TcpClient.ts**: Raw TCP socket connection with packet framing and handshake

### Authentication Layer (`src/renderer/src/services/auth/`)

- **AuthService.ts**: High-level authentication API (login, signup, logout)

## Protocol Implementation

### Packet Format

All packets follow this structure (matching server's `protocol.c`):

```
┌─────────────┬──────────────┬──────────────┬─────────────┐
│ packet_len  │ protocol_ver │ packet_type  │  payload    │
│  (2 bytes)  │  (1 byte)    │  (2 bytes)   │  (variable) │
└─────────────┴──────────────┴──────────────┴─────────────┘
```

- **Big-endian** byte order (network byte order)
- **MessagePack** serialization for payloads
- **5-byte header** + variable payload

### Handshake

Before regular packets, client performs a handshake:

1. Client sends: `[length:2, protocol_version:2]`
2. Server responds: `[length:2, code:1]`
   - `0x00` = OK
   - `0x01` = Protocol not supported

### Authentication Flow

#### Login
1. Client encodes: `{user: "username", pass: "password"}`
2. Sends packet type `100` (LOGIN_REQUEST)
3. Receives packet type `100` (LOGIN_RESPONSE)
4. Response contains: `{res: 101, user: {...}}` on success

#### Signup
1. Client encodes full signup request with all fields
2. Sends packet type `200` (SIGNUP_REQUEST)
3. Receives packet type `200` (SIGNUP_RESPONSE)
4. Response contains: `{res: 201}` on success

### Validation

Client-side validation matches server requirements:

**Username**:
- At least 5 characters
- Alphanumeric + underscores only

**Password**:
- At least 10 characters
- Must contain both letters and numbers

## Usage

### Login Example

```typescript
import { AuthService } from './services/auth';

const authService = new AuthService({
  host: 'localhost',
  port: 8080
});

try {
  const user = await authService.login('username', 'password');
  console.log('Logged in:', user);
} catch (error) {
  console.error('Login failed:', error);
}
```

### Signup Example

```typescript
const signupRequest = {
  user: 'newuser',
  pass: 'securepass123',
  email: 'user@example.com',
  phone: '1234567890',
  fullname: 'John Doe',
  country: 'USA',
  gender: 'Male',
  dob: '1990-01-01'
};

await authService.signup(signupRequest);
```

## Components

### LoginPage
- Updated to use async authentication
- Shows loading state during login
- Displays connection errors
- Link to signup page

### SignupPage (New)
- Complete registration form with validation
- Two-column responsive layout
- Real-time field validation
- Redirects to login on success

### AuthContext
- Manages authentication state
- Maintains TCP connection
- Stores user session
- Provides signup method

## Configuration

### Electron Main Process
- **nodeIntegration**: `true` - Required for TCP sockets
- **contextIsolation**: `false` - Simplifies Node.js access
- Socket communication uses Node.js `net` module

### TypeScript
- **esModuleInterop**: `true` - Fixed React import issues
- Strict type checking enabled
- All protocol types strongly typed

## Testing

To test the implementation:

1. Start the server:
   ```bash
   cd /home/datpham-nuoa/dev/Cardio/server/build
   ./Cardio_server
   ```

2. Start the client:
   ```bash
   cd /home/datpham-nuoa/dev/Cardio/poker-client
   yarn dev
   ```

3. Try logging in with existing credentials or creating a new account

## Security Notes

⚠️ **Production Considerations**:
- Current implementation uses plain TCP (no TLS)
- Passwords sent in plaintext over network
- For production, implement TLS/SSL encryption
- Consider using secure WebSocket (wss://) as alternative
- Add CSRF protection for web deployment

## File Structure

```
poker-client/src/renderer/src/
├── services/
│   ├── protocol/
│   │   ├── constants.ts    # Protocol constants
│   │   ├── types.ts        # TypeScript interfaces
│   │   ├── codec.ts        # Encode/decode functions
│   │   └── index.ts
│   ├── network/
│   │   ├── TcpClient.ts    # TCP socket wrapper
│   │   └── index.ts
│   └── auth/
│       ├── AuthService.ts  # Authentication API
│       └── index.ts
├── contexts/
│   └── AuthContext.tsx     # Auth state management
├── pages/
│   ├── LoginPage.tsx       # Login UI
│   └── SignupPage.tsx      # Signup UI (new)
└── types/
    └── index.ts            # Updated types
```

## Dependencies

- **@msgpack/msgpack**: MessagePack serialization library
- **net** (Node.js): TCP socket communication (via Electron)

## Next Steps

Future enhancements:
1. Implement game-specific packet handlers (table join, actions, etc.)
2. Add reconnection logic with exponential backoff
3. Implement heartbeat/keepalive mechanism
4. Add packet queueing for offline scenarios
5. Implement TLS encryption for production
6. Add comprehensive error handling and retry logic
