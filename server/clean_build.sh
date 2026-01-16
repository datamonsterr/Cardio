#!/bin/sh

# Set the base directory to the 'lib' folder
BASE_DIR="lib"

# Check if the base directory exists
if [ ! -d "$BASE_DIR" ]; then
    echo "Directory '$BASE_DIR' does not exist."
    exit 1
fi

# Iterate through each folder in the base directory
for LIB_DIR in "$BASE_DIR"/*; do
    if [ -d "$LIB_DIR" ]; then
        BUILD_DIR="$LIB_DIR/build"

        if [ -d "$BUILD_DIR" ]; then
            echo "Cleaning build directory: $BUILD_DIR"
            rm -rf "$BUILD_DIR"
        else
            echo "No build directory found in $LIB_DIR"
        fi
    fi
done

echo "All build directories cleaned."