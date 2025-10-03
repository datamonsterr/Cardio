-- Migration script to update password storage from plaintext to hashed passwords
-- This script updates the password column to support hashed passwords (SHA-512)

-- Step 1: Add a temporary column for the new hashed passwords
ALTER TABLE "User" ADD COLUMN password_hashed VARCHAR(128);

-- Step 2: Update the constraint to allow temporary longer passwords
ALTER TABLE "User" DROP CONSTRAINT IF EXISTS "User_password_check";

-- Step 3: Modify the password column to support longer hashed passwords
ALTER TABLE "User" ALTER COLUMN password TYPE VARCHAR(128);

-- Step 4: Drop the temporary column (we'll hash in application)
ALTER TABLE "User" DROP COLUMN IF EXISTS password_hashed;

-- Step 5: Add comment to document the change
COMMENT ON COLUMN "User".password IS 'SHA-512 hashed password with salt (max 128 chars)';

-- Note: Existing plaintext passwords in the database will need to be rehashed
-- This is handled by the migration helper script (migrate_existing_data.sh)
