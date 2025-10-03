# Password Security Migration Guide

## Overview

This guide explains how to migrate from plaintext password storage to secure hashed passwords using SHA-512 with salt.

## Security Improvements

### Before (Insecure)
- Passwords stored in plaintext
- Direct string comparison for authentication
- Vulnerable to database breaches

### After (Secure)
- Passwords hashed using SHA-512 with unique salt per user
- Industry-standard cryptographic hashing
- Even with database access, passwords cannot be recovered

## Migration Process

### Step 1: Backup Your Database

**CRITICAL: Always backup before migration!**

```bash
# Create a backup of your database
pg_dump -U postgres -h localhost -p 5433 cardio > cardio_backup_$(date +%Y%m%d).sql

# Or backup just the User table
pg_dump -U postgres -h localhost -p 5433 -t "User" cardio > user_table_backup.sql
```

### Step 2: Update Database Schema

Run the schema migration to allow longer password hashes:

```bash
# Connect to your database
psql -U postgres -h localhost -p 5433 -d cardio -f migration_hash_passwords.sql
```

This script:
- Changes the `password` column from VARCHAR(32) to VARCHAR(128)
- Removes the old password length constraint
- Adds documentation comments

### Step 3: Build the Migration Tool

Compile the password migration program:

```bash
cd database
gcc -o migrate_passwords migrate_passwords.c -lpq -lcrypt
```

### Step 4: Migrate Existing Passwords

Run the migration tool to hash all existing plaintext passwords:

```bash
# Using default connection (from migrate_passwords.c)
./migrate_passwords

# Or specify a custom connection string
./migrate_passwords "dbname=cardio user=postgres password=1234 host=localhost port=5433"
```

The tool will:
- Connect to the database
- Find all users with plaintext passwords
- Generate a unique salt for each password
- Hash each password using SHA-512
- Update the database with hashed passwords
- Report migration status

**Example Output:**
```
Password Migration Tool
========================

Connecting to database...
Connected successfully!

Found 50 users to process

User 1: Password hashed successfully
User 2: Password hashed successfully
...
User 50: Password hashed successfully

========================
Migration Summary:
  Migrated: 50
  Skipped:  0 (already hashed)
  Failed:   0
========================
```

### Step 5: Rebuild the Backend

After migration, rebuild the backend with the new secure authentication:

```bash
cd ../backend/lib/db
rm -rf build
mkdir build
cd build
cmake ..
make

# Return to main backend
cd ../../..
rm -rf build
mkdir build
cd build
cmake ..
make
```

### Step 6: Test Authentication

Test that login still works with the hashed passwords:

```bash
# Start your server
./Kasino_server

# Test login with existing credentials
# The password verification will now use hash comparison
```

## How It Works

### Password Hashing (Signup)

When a new user signs up:

1. User provides plaintext password
2. System generates a random salt (16 characters)
3. Password is hashed using SHA-512 with the salt
4. Hash (containing salt) is stored in database
5. Plaintext password is never stored

```c
// Example hash format:
$6$randomsalt1234$hashedpasswordstring...
│  │               │
│  │               └─ Actual hash (86 characters)
│  └─ Salt (16 characters)
└─ Algorithm identifier ($6$ = SHA-512)
```

### Password Verification (Login)

When a user logs in:

1. User provides plaintext password
2. System retrieves stored hash from database
3. Extracts salt from stored hash
4. Hashes provided password with same salt
5. Compares hashes (constant-time comparison)
6. Authentication succeeds only if hashes match

## Rollback Procedure

If you need to rollback (only if you have a backup!):

```bash
# Restore from backup
psql -U postgres -h localhost -p 5433 -d cardio < cardio_backup_YYYYMMDD.sql

# Or restore just the User table
psql -U postgres -h localhost -p 5433 -d cardio -c "TRUNCATE \"User\" CASCADE;"
psql -U postgres -h localhost -p 5433 -d cardio < user_table_backup.sql
```

## Troubleshooting

### Migration tool fails to compile

Make sure you have required libraries:
```bash
sudo apt-get install libpq-dev libcrypt-dev
```

### "Connection to database failed"

Check your connection parameters:
- Database name: `cardio`
- User: `postgres`
- Password: `1234`
- Host: `localhost`
- Port: `5433`

Modify the `conninfo` in `migrate_passwords.c` or pass as argument.

### "Password hashing failed"

Ensure libcrypt is installed:
```bash
ls -la /usr/lib/x86_64-linux-gnu/libcrypt.so*
```

### Login fails after migration

1. Check that backend was rebuilt with new code
2. Verify passwords were actually hashed (check database)
3. Test with a fresh signup (should work immediately)

## Testing Checklist

After migration, verify:

- [ ] Existing users can login with their passwords
- [ ] New user signup creates hashed password
- [ ] New users can login immediately after signup
- [ ] Password validation rules still work
- [ ] Database schema updated correctly
- [ ] All 50 test users migrated successfully

## Security Notes

### Password Requirements (Still Enforced)

- Minimum 10 characters
- Must contain both letters and numbers
- Validated before hashing

### Best Practices

1. **Never log plaintext passwords** - Even during debugging
2. **Never transmit passwords over unencrypted connections** - Use TLS/SSL
3. **Rate limit login attempts** - Prevent brute force attacks
4. **Use secure password reset** - Don't email passwords
5. **Regular security audits** - Review authentication code

## Additional Security Improvements (Future)

Consider implementing:

1. **Multi-factor authentication (MFA)**
2. **Password complexity requirements** (special characters, etc.)
3. **Password expiration policies**
4. **Account lockout after failed attempts**
5. **Secure session management**
6. **TLS/SSL encryption for connections**

## Support

If you encounter issues:

1. Check the migration tool output for specific errors
2. Verify database connection settings
3. Ensure all dependencies are installed
4. Review the backup before attempting rollback

## Files Modified

- `backend/lib/db/include/db.h` - Added password hashing functions
- `backend/lib/db/src/password_hash.c` - Password hashing implementation
- `backend/lib/db/src/login.c` - Updated to verify hashed passwords
- `backend/lib/db/src/signup.c` - Updated to hash passwords before storage
- `backend/lib/db/CMakeLists.txt` - Added libcrypt dependency
- `database/migration_hash_passwords.sql` - Schema migration
- `database/migrate_passwords.c` - Password migration tool

## References

- [crypt(3) Linux man page](https://man7.org/linux/man-pages/man3/crypt.3.html)
- [SHA-512 Specification](https://en.wikipedia.org/wiki/SHA-2)
- [OWASP Password Storage Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html)
