#!/bin/bash

# Integration test runner script
set -e

echo "=== Integration Test Suite ==="
echo ""

# Ensure build directory exists
mkdir -p build

# Check if integration test binary exists
if [ ! -f "build/integration_test" ]; then
    echo "Building integration test binary..."
    cd build
    
    # Compile integration test
    clang -std=gnu17 -Wall \
        -I ../include \
        -I ../lib/card/include \
        -I ../lib/pokergame/include \
        -I ../lib/mpack/include \
        -I ../lib/db/include \
        -I ../lib/logger/include \
        -I ../lib/utils \
        -isystem /usr/include/postgresql \
        ../test/integration_test.c \
        ../src/protocol.c \
        ../src/utils.c \
        ../src/game_room.c \
        ../src/handler.c \
        ../src/server.c \
        -o integration_test \
        ../lib/card/build/libCardio_card.a \
        ../lib/pokergame/build/libCardio_pokergame.a \
        ../lib/mpack/build/libCardio_mpack.a \
        ../lib/db/build/libCardio_db.a \
        ../lib/logger/build/libCardio_logger.a \
        -lpq -lcrypt
    
    cd ..
fi

# Run integration tests
echo "Running integration tests..."
cd build
./integration_test

echo ""
echo "=== All Integration Tests Passed ==="
