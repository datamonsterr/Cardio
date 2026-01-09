# Cardio Database

This directory contains database schemas, migrations, and related tools.

## Files

### Schema Files
- **schemas.sql** - Initial database schema definition
- **data.sql** - Sample/test data for development

### Security Migration (New!)
- **migration_hash_passwords.sql** - SQL script to update password column schema
- **migrate_passwords.c** - C program to hash existing plaintext passwords
- **run_migration.sh** - Automated migration script (recommended)
- **MIGRATION_GUIDE.md** - Comprehensive migration documentation

## Quick Start

### For New Installations

If you're setting up a fresh database:

```bash
# Create database
createdb -U postgres -h localhost -p 5433 cardio

# Load schema
psql -U postgres -h localhost -p 5433 -d cardio -f schemas.sql

# Load schema with password support already included
psql -U postgres -h localhost -p 5433 -d cardio -f migration_hash_passwords.sql

# (Optional) Load test data - Note: passwords will need to be hashed
psql -U postgres -h localhost -p 5433 -d cardio -f data.sql
```

### For Existing Installations

If you have an existing database with plaintext passwords:

```bash
# Run the automated migration
./run_migration.sh
```

This will:
1. Backup your database
2. Update the schema
3. Compile the migration tool
4. Hash all existing passwords
5. Rebuild the server

See **MIGRATION_GUIDE.md** for detailed instructions and troubleshooting.

## Database Schema

### User Table

```sql
CREATE TABLE "User" (
    user_id  SERIAL PRIMARY KEY,
    email    VARCHAR(100) NOT NULL UNIQUE,
    phone    VARCHAR(15)  NOT NULL,
    dob      DATE         NOT NULL,
    password VARCHAR(128) NOT NULL,  -- SHA-512 hashed (was VARCHAR(32))
    country  VARCHAR(32),
    gender   VARCHAR(10)  NOT NULL CHECK (gender IN ('Male', 'Female', 'Other')),
    balance  INTEGER DEFAULT 0,
    registration_date DATE DEFAULT CURRENT_DATE,
    avatar_url VARCHAR(100),
    full_name VARCHAR(100) NOT NULL DEFAULT '',
    username VARCHAR(100) NOT NULL UNIQUE
);
```

### Friend Table

```sql
CREATE TABLE Friend (
    u1 INTEGER NOT NULL,
    u2 INTEGER NOT NULL,
    FOREIGN KEY (u1) REFERENCES "User"(user_id),
    FOREIGN KEY (u2) REFERENCES "User"(user_id),
    PRIMARY KEY (u1, u2)
);
```

## Security Features

### Password Hashing

Passwords are now hashed using **SHA-512 with salt**:

- Each password has a unique 16-character salt
- Hashes are 106 characters long
- Industry-standard security
- Thread-safe implementation

**Hash Format:**
```
$6$randomsalt1234$hashedpassworddata...
│  │               │
│  │               └─ Hashed password (86 chars)
│  └─ Random salt (16 chars)
└─ Algorithm ($6$ = SHA-512)
```

### Password Requirements

- Minimum 10 characters
- Must contain both letters and numbers
- Username must be at least 5 characters
- Username must be alphanumeric only

## Development

### Connection Info

Default connection parameters (defined in `migrate_passwords.c` and server):

```
Database: cardio
User:     postgres
Password: 1234
Host:     localhost
Port:     5433
```

To use different parameters, either:
1. Modify the source files
2. Pass connection string as argument to `migrate_passwords`
3. Set environment variables before running `run_migration.sh`

### Testing Migration Tool

Compile and test:

```bash
gcc -o migrate_passwords migrate_passwords.c -I/usr/include/postgresql -lpq -lcrypt
./migrate_passwords "dbname=cardio user=postgres password=1234 host=localhost port=5433"
```

## Backup and Recovery

### Creating Backups

```bash
# Full database backup
pg_dump -U postgres -h localhost -p 5433 cardio > backup.sql

# User table only
pg_dump -U postgres -h localhost -p 5433 -t "User" cardio > user_backup.sql
```

### Restoring Backups

```bash
# Full restore
psql -U postgres -h localhost -p 5433 -d cardio < backup.sql

# Table restore
psql -U postgres -h localhost -p 5433 -d cardio < user_backup.sql
```

## Troubleshooting

### PostgreSQL Connection Issues

If you can't connect:

```bash
# Check if PostgreSQL is running
sudo systemctl status postgresql

# Test connection
psql -U postgres -h localhost -p 5433 -d cardio -c "SELECT version();"
```

### Migration Issues

See `MIGRATION_GUIDE.md` for detailed troubleshooting steps.

## References

- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [Password Hashing Best Practices](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html)
- [SHA-512 Specification](https://en.wikipedia.org/wiki/SHA-2)
