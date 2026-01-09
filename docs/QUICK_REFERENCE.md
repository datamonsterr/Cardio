# Quick Reference Guide

## Common Tasks

### Building

```bash
# Build everything (all libraries and main project)
make build

# Build just the libraries
cd server && ./build_all.sh

# Build a specific library
cd server/lib/card
mkdir -p build && cd build
cmake ..
make
```

### Testing

```bash
# Run all library tests (recommended for quick checks)
make test-libs

# Run all tests including database tests
make test

# Run a specific library test
cd server/lib/card/build
./Cardio_card_test

cd server/lib/logger/build
./Cardio_logger_test

cd server/lib/pokergame/build
./Cardio_pokergame_test
```

### Code Quality

```bash
# Format all C code
make format

# Check if code is properly formatted (doesn't modify files)
find server -type f \( -name "*.c" -o -name "*.h" \) ! -path "*/build/*" ! -path "*/mpack/*" -exec clang-format --dry-run --Werror {} +

# Lint all C code
make lint
```

### Cleaning

```bash
# Clean all build artifacts
make clean

# Clean and rebuild everything
make clean && make build
```

### Help

```bash
# Show all available make targets
make help
```

## Test Results Interpretation

### Success
```
================================
All tests PASSED
```
Exit code: 0

### Failure
```
================================
Some tests FAILED
```
Exit code: 1

### Individual Test Results
- `PASS: <assertion>` - Test passed
- `FAIL: <assertion>` - Test failed

## File Locations

### Tests
- Card library: `server/lib/card/test/unit_tests.c`
- Logger library: `server/lib/logger/test/unit_tests.c`
- Pokergame library: `server/lib/pokergame/test/unit_tests.c`
- Database library: `server/lib/db/test/unit_test.c`
- Main project: `server/test/unit_test.c`

### Test Executables (after build)
- Card: `server/lib/card/build/Cardio_card_test`
- Logger: `server/lib/logger/build/Cardio_logger_test`
- Pokergame: `server/lib/pokergame/build/Cardio_pokergame_test`
- Database: `server/lib/db/build/Cardio_db_test`
- Main: `server/build/test`

### Configuration Files
- `.clang-format` - Code formatting style
- `.clang-tidy` - Static analysis configuration
- `.github/workflows/ci.yml` - GitHub Actions CI pipeline

### Documentation
- `TESTING.md` - Comprehensive testing guide
- `CI_SETUP_SUMMARY.md` - Summary of CI setup
- `README.md` - Main project documentation
- `QUICK_REFERENCE.md` - This file

## Common Issues

### Tests Won't Build
1. Make sure all libraries are built first: `cd server && ./build_all.sh`
2. Check CMakeLists.txt has test target configured
3. Verify all dependencies are installed

### Tests Fail
1. Check test output for specific assertion that failed
2. Verify library is working: run simple test first
3. For database tests, ensure PostgreSQL is running

### Formatting Issues
1. Run `make format` to auto-fix formatting
2. Check `.clang-format` for style configuration
3. Exclude generated/vendor code (like mpack)

### Linting Warnings
1. Linting warnings are informational, not errors
2. Configure `.clang-tidy` to disable specific checks
3. Use `continue-on-error: true` in CI for linting

## CI Pipeline

When you push code, GitHub Actions automatically:

1. ✓ Installs dependencies (clang, cmake, PostgreSQL)
2. ✓ Builds all libraries
3. ✓ Builds main project
4. ✓ Runs all library tests
5. ✓ Checks code formatting
6. ✓ Runs static analysis

View results at: `https://github.com/datamonsterr/Cardio/actions`

## Quick Test Addition

To add a new test to a library:

```c
// In server/lib/mylib/test/unit_tests.c

TEST(test_my_new_feature)
{
    // Arrange
    int input = 10;
    
    // Act
    int result = my_function(input);
    
    // Assert
    ASSERT(result == 20);
}

int main()
{
    // ... other tests ...
    RUN_TEST(test_my_new_feature);  // Add this line
    return failed;
}
```

Then rebuild and run:
```bash
cd server/lib/mylib/build
make
./Cardio_mylib_test
```

## Performance Tips

- Use `make test-libs` for quick iteration (skips database tests)
- Build individual libraries instead of all when developing
- Use `make clean` sparingly (rebuilding takes time)
- Run formatting before committing to avoid CI failures

## Getting Help

1. Check `TESTING.md` for detailed testing documentation
2. Check `CI_SETUP_SUMMARY.md` for overview of the setup
3. Read test output carefully - it shows which assertion failed
4. Review existing tests as examples
5. Ask the team for guidance
