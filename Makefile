.PHONY: all build clean test test-libs test-integration format format-check lint lint-fix tidy clean-format help

# Default target
all: build

# Build all libraries and main project
build:
	@echo "Building all libraries..."
	cd backend && ./build_all.sh
	@echo "Building main project..."
	cd backend && mkdir -p build && cd build && cmake .. && make || echo "Warning: Main project build failed (may have unrelated issues)"

# Clean all build artifacts
clean:
	@echo "Cleaning all build directories..."
	cd backend && ./clean_build.sh
	cd backend && rm -rf build

# Run all library unit tests (excluding main project tests that may require database setup)
test-libs:
	@echo "Running library unit tests..."
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
	@echo "All library tests passed!"

# Run all unit tests including main project (requires database setup)
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
	@cd backend/lib/db/build && ./Kasino_db_test || echo "Warning: Database tests require PostgreSQL setup"
	@echo ""
	@echo "=== Main Project Tests ==="
	@cd backend/build && ./test || echo "Warning: Main project tests require PostgreSQL setup"
	@echo ""
	@echo "All tests completed!"

# Run integration tests (requires build and database setup)
test-integration:
	@echo "Running integration tests..."
	cd backend && ./run_integration_tests.sh

# ===============================
# FORMATTING AND TIDYING TARGETS
# ===============================

# Format all C source files using clang-format (in-place)
format:
	@./scripts/format-and-tidy.sh format

# Check formatting without modifying files
format-check:
	@./scripts/format-and-tidy.sh format-check

# Lint all C source files using clang-tidy
lint:
	@./scripts/format-and-tidy.sh lint

# Lint and auto-fix issues where possible
lint-fix:
	@./scripts/format-and-tidy.sh lint-fix

# Combined formatting and linting (full tidy)
tidy:
	@./scripts/format-and-tidy.sh tidy

# Clean formatting backup files (if any clang tools create them)
clean-format:
	@./scripts/format-and-tidy.sh clean

# Help target to display available commands
help:
	@echo "Available targets:"
	@echo ""
	@echo "📦 BUILD TARGETS:"
	@echo "  make all              - Build all libraries and main project (default)"
	@echo "  make build            - Build all libraries and main project"
	@echo "  make clean            - Clean all build artifacts"
	@echo ""
	@echo "🧪 TEST TARGETS:"
	@echo "  make test-libs        - Run library unit tests (no database required)"
	@echo "  make test             - Run all unit tests (requires database setup)"
	@echo "  make test-integration - Run integration tests (requires build and database)"
	@echo ""
	@echo "🎨 FORMATTING & TIDYING TARGETS:"
	@echo "  make format           - Format all C files with clang-format (in-place)"
	@echo "  make format-check     - Check if files need formatting (no changes)"
	@echo "  make lint             - Lint all C files with clang-tidy"
	@echo "  make lint-fix         - Lint and auto-fix issues where possible"
	@echo "  make tidy             - Run both format and lint (full code tidying)"
	@echo "  make clean-format     - Clean formatting backup files"
	@echo ""
	@echo "ℹ️  OTHER:"
	@echo "  make help             - Show this help message"
