#!/usr/bin/env bash

# Set the base directory to the 'lib' folder
BASE_DIR="lib"

# Directory to exclude
EXCLUDE_DIR="lib/utils"

# Check if the base directory exists
if [ ! -d "$BASE_DIR" ]; then
    echo "Directory '$BASE_DIR' does not exist."
    exit 1
fi

# Iterate through each folder in the base directory
for LIB_DIR in "$BASE_DIR"/*; do
    if [ -d "$LIB_DIR" ]; then
        # Skip the excluded directory
        if [ "$LIB_DIR" == "$EXCLUDE_DIR" ]; then
            echo "Skipping excluded directory: $LIB_DIR"
            continue
        fi

        echo "Processing library: $LIB_DIR"

        # Create build directory inside the library folder
        BUILD_DIR="$LIB_DIR/build"
        mkdir -p "$BUILD_DIR"

        # Navigate to the build directory
        cd "$BUILD_DIR" || exit

        # Run cmake and make
        echo "Running cmake in $BUILD_DIR..."
        if cmake ..; then
            echo "Running make in $BUILD_DIR..."
            if make; then
                echo "Build successful for $LIB_DIR"
            else
                echo "Make failed for $LIB_DIR"
                exit 1
            fi
        else
            echo "CMake configuration failed for $LIB_DIR"
            exit 1
        fi

        # Return to the base directory
        cd - > /dev/null || exit
    fi
done

echo "All libraries processed."