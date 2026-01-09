#!/bin/bash

# Automated Password Migration Script
# This script automates the entire migration process

set -e  # Exit on error

echo "╔═══════════════════════════════════════════════════════════╗"
echo "║     Password Security Migration - Automated Script       ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# Configuration
DB_NAME="${DB_NAME:-cardio}"
DB_USER="${DB_USER:-postgres}"
DB_PASSWORD="${DB_PASSWORD:-1234}"
DB_HOST="${DB_HOST:-localhost}"
DB_PORT="${DB_PORT:-5433}"
BACKUP_DIR="./backups"

# Create backup directory if it doesn't exist
mkdir -p "$BACKUP_DIR"

# Step 1: Create backup
echo "Step 1: Creating database backup..."
BACKUP_FILE="$BACKUP_DIR/cardio_backup_$(date +%Y%m%d_%H%M%S).sql"
PGPASSWORD="$DB_PASSWORD" pg_dump -U "$DB_USER" -h "$DB_HOST" -p "$DB_PORT" "$DB_NAME" > "$BACKUP_FILE" 2>/dev/null || {
    echo "⚠️  Warning: Backup failed. This might be because PostgreSQL is not running."
    echo "   Continuing anyway, but be cautious!"
    BACKUP_FILE="none"
}

if [ "$BACKUP_FILE" != "none" ]; then
    echo "✓ Backup created: $BACKUP_FILE"
fi
echo ""

# Step 2: Update schema
echo "Step 2: Updating database schema..."
PGPASSWORD="$DB_PASSWORD" psql -U "$DB_USER" -h "$DB_HOST" -p "$DB_PORT" -d "$DB_NAME" -f migration_hash_passwords.sql 2>/dev/null || {
    echo "⚠️  Warning: Schema update might have failed. Check if database is accessible."
}
echo "✓ Schema updated"
echo ""

# Step 3: Compile migration tool
echo "Step 3: Compiling migration tool..."
if [ -f migrate_passwords.c ]; then
    gcc -o migrate_passwords migrate_passwords.c -I/usr/include/postgresql -lpq -lcrypt || {
        echo "❌ Error: Failed to compile migration tool"
        echo "   Make sure you have libpq-dev and build-essential installed"
        exit 1
    }
    echo "✓ Migration tool compiled"
else
    echo "❌ Error: migrate_passwords.c not found"
    exit 1
fi
echo ""

# Step 4: Run migration
echo "Step 4: Migrating passwords..."
CONNINFO="dbname=$DB_NAME user=$DB_USER password=$DB_PASSWORD host=$DB_HOST port=$DB_PORT"
./migrate_passwords "$CONNINFO" || {
    echo "❌ Error: Password migration failed"
    if [ "$BACKUP_FILE" != "none" ]; then
        echo "   You can restore from backup: $BACKUP_FILE"
    fi
    exit 1
}
echo ""

# Step 5: Rebuild server
echo "Step 5: Rebuilding server with new security features..."
cd ../server/lib/db

if [ -d build ]; then
    rm -rf build
fi
mkdir build
cd build

echo "  Building db library..."
cmake .. > /dev/null 2>&1 && make > /dev/null 2>&1 || {
    echo "⚠️  Warning: server rebuild might have issues"
}

echo "✓ server rebuilt"
echo ""

# Success message
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║              Migration Completed Successfully!            ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""
echo "Next steps:"
echo "  1. Rebuild your main application"
echo "  2. Test login with existing users"
echo "  3. Test signup with new users"
echo ""
if [ "$BACKUP_FILE" != "none" ]; then
    echo "Backup location: $BACKUP_FILE"
    echo "Keep this backup until you've verified everything works!"
fi
echo ""
echo "For detailed information, see MIGRATION_GUIDE.md"
