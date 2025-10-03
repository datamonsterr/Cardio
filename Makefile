.PHONY: all build clean test format lint help

# Default target
all: build

# Build all libraries and main project
build:
	@echo "Building all libraries..."
	cd backend && ./build_all.sh
	@echo "Building main project..."
	cd backend && mkdir -p build && cd build && cmake .. && make

# Clean all build artifacts
clean:
	@echo "Cleaning all build directories..."
	cd backend && ./clean_build.sh
	cd backend && rm -rf build

# Run all unit tests
test:
	@echo "Running all unit tests..."
	@echo ""
	@echo "=== Card Library Tests ==="
	@cd backend/lib/card/build && ./Kasino_card_test || exit 1
	@echo ""
	@echo "=== Logger Library Tests ==="
	@cd backend/lib/logger/build && ./Kasino_logger_test || exit 1
	@echo ""
	@echo "=== Pokergame Library Tests ==="
	@cd backend/lib/pokergame/build && ./Kasino_pokergame_test || exit 1
	@echo ""
	@echo "=== Database Library Tests ==="
	@cd backend/lib/db/build && ./Kasino_db_test || exit 1
	@echo ""
	@echo "=== Main Project Tests ==="
	@cd backend/build && ./test || exit 1
	@echo ""
	@echo "All tests passed!"

# Format all C source files using clang-format
format:
	@echo "Formatting all C files with clang-format..."
	@find backend -type f \( -name "*.c" -o -name "*.h" \) ! -path "*/build/*" ! -path "*/mpack/*" -exec clang-format -i {} +
	@echo "Formatting complete!"

# Lint all C source files using clang-tidy
lint:
	@echo "Linting all C files with clang-tidy..."
	@find backend/lib/card/src -type f -name "*.c" -exec clang-tidy {} -- -I backend/lib/card/include \; || true
	@find backend/lib/logger/src -type f -name "*.c" -exec clang-tidy {} -- -I backend/lib/logger/include \; || true
	@find backend/lib/pokergame/src -type f -name "*.c" -exec clang-tidy {} -- -I backend/lib/pokergame/include -I backend/lib/card/include -I backend/lib/utils \; || true
	@find backend/lib/db/src -type f -name "*.c" -exec clang-tidy {} -- -I backend/lib/db/include -I /usr/include/postgresql \; || true
	@echo "Linting complete!"

# Help target to display available commands
help:
	@echo "Available targets:"
	@echo "  make all     - Build all libraries and main project (default)"
	@echo "  make build   - Build all libraries and main project"
	@echo "  make clean   - Clean all build artifacts"
	@echo "  make test    - Run all unit tests"
	@echo "  make format  - Format all C files with clang-format"
	@echo "  make lint    - Lint all C files with clang-tidy"
	@echo "  make help    - Show this help message"
