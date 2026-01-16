#!/bin/sh

# Set the base directory to the 'lib' folder
BASE_DIR="lib"

# Directory to exclude
EXCLUDE_DIR="lib/utils"

# Check if the base directory exists
if [ ! -d "$BASE_DIR" ]; then
    echo "Directory '$BASE_DIR' does not exist."
    exit 1
fi

# Define build order based on dependencies:
# logger: no dependencies
# card: no dependencies
# mpack: no dependencies
# db: depends on logger
# pokergame: depends on card
BUILD_ORDER=("logger" "card" "mpack" "db" "pokergame")

# Function to build a library
build_library() {
    local LIB_NAME=$1
    local LIB_DIR="$BASE_DIR/$LIB_NAME"
    
    if [ ! -d "$LIB_DIR" ]; then
        echo "Warning: Library directory '$LIB_DIR' does not exist, skipping..."
        return 0
    fi
    
    echo "Processing library: $LIB_DIR"
    
    # Create build directory inside the library folder
    BUILD_DIR="$LIB_DIR/build"
    mkdir -p "$BUILD_DIR"
    
    # Navigate to the build directory
    cd "$BUILD_DIR" || exit 1
    
    # Run cmake and make
    echo "Running cmake in $BUILD_DIR..."
    if cmake ..; then
        echo "Running make in $BUILD_DIR..."
        if make; then
            echo "Build successful for $LIB_DIR"
        else
            echo "Make failed for $LIB_DIR"
            cd - > /dev/null || exit 1
            exit 1
        fi
    else
        echo "CMake configuration failed for $LIB_DIR"
        cd - > /dev/null || exit 1
        exit 1
    fi
    
    # Return to the base directory
    cd - > /dev/null || exit 1
}

# Build libraries in dependency order
for LIB_NAME in "${BUILD_ORDER[@]}"; do
    build_library "$LIB_NAME"
done

echo "All libraries processed."