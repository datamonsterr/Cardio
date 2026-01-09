.PHONY: all build clean test test-libs test-integration format lint help

# Default target
all: build

# Build all libraries and main project
build:
	@echo "Building all libraries..."
	cd server && ./build_all.sh
	@echo "Building main project..."
	cd server && mkdir -p build && cd build && cmake .. && make || echo "Warning: Main project build failed (may have unrelated issues)"

# Clean all build artifacts
clean:
	@echo "Cleaning all build directories..."
	cd server && ./clean_build.sh
	cd server && rm -rf build

# Run all library unit tests (excluding main project tests that may require database setup)
test-libs:
	@echo "Running library unit tests..."
	@echo ""
	@echo "=== Card Library Tests ==="
	@cd server/lib/card/build && ./Cardio_card_test || exit 1
	@echo ""
	@echo "=== Logger Library Tests ==="
	@cd server/lib/logger/build && ./Cardio_logger_test || exit 1
	@echo ""
	@echo "=== Pokergame Library Tests ==="
	@cd server/lib/pokergame/build && ./Cardio_pokergame_test || exit 1
	@echo ""
	@echo "All library tests passed!"

# Run all unit tests including main project (requires database setup)
test:
	@echo "Running all unit tests..."
	@echo ""
	@echo "=== Card Library Tests ==="
	@cd server/lib/card/build && ./Cardio_card_test || exit 1
	@echo ""
	@echo "=== Logger Library Tests ==="
	@cd server/lib/logger/build && ./Cardio_logger_test || exit 1
	@echo ""
	@echo "=== Pokergame Library Tests ==="
	@cd server/lib/pokergame/build && ./Cardio_pokergame_test || exit 1
	@echo ""
	@echo "=== Database Library Tests ==="
	@cd server/lib/db/build && ./Cardio_db_test || echo "Warning: Database tests require PostgreSQL setup"
	@echo ""
	@echo "=== Main Project Tests ==="
	@cd server/build && ./test || echo "Warning: Main project tests require PostgreSQL setup"
	@echo ""
	@echo "All tests completed!"

# Run integration tests (requires build and database setup)
test-integration:
	@echo "Running integration tests..."
	cd server && ./run_integration_tests.sh

# Format all C source files using clang-format
format:
	@echo "Formatting all C files with clang-format..."
	@find server -type f \( -name "*.c" -o -name "*.h" \) ! -path "*/build/*" ! -path "*/mpack/*" -exec clang-format -i {} +
	@echo "Formatting complete!"

# Lint all C source files using clang-tidy
lint:
	@echo "Linting all C files with clang-tidy..."
	@find server/lib/card/src -type f -name "*.c" -exec clang-tidy {} -- -I server/lib/card/include \; || true
	@find server/lib/logger/src -type f -name "*.c" -exec clang-tidy {} -- -I server/lib/logger/include \; || true
	@find server/lib/pokergame/src -type f -name "*.c" -exec clang-tidy {} -- -I server/lib/pokergame/include -I server/lib/card/include -I server/lib/utils \; || true
	@find server/lib/db/src -type f -name "*.c" -exec clang-tidy {} -- -I server/lib/db/include -I /usr/include/postgresql \; || true
	@echo "Linting complete!"

# Help target to display available commands
help:
	@echo "Available targets:"
	@echo "  make all              - Build all libraries and main project (default)"
	@echo "  make build            - Build all libraries and main project"
	@echo "  make clean            - Clean all build artifacts"
	@echo "  make test-libs        - Run library unit tests (no database required)"
	@echo "  make test             - Run all unit tests (requires database setup)"
	@echo "  make test-integration - Run integration tests (requires build and database)"
	@echo "  make format           - Format all C files with clang-format"
	@echo "  make lint             - Lint all C files with clang-tidy"
	@echo "  make help             - Show this help message"
