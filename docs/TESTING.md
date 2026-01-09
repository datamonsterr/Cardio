# Testing Documentation

This document describes the testing infrastructure and guidelines for the Cardio project.

## Overview

The Cardio project uses a custom lightweight testing framework defined in `server/lib/utils/testing.h`. Each library has its own set of unit tests that validate the functionality of its main functions.

## Testing Framework

### Test Macros

The testing framework provides the following macros:

- `TEST(name)` - Define a test function
- `RUN_TEST(name)` - Execute a test function
- `ASSERT(expr)` - Assert that an expression is true
- `ASSERT_STR_EQ(str1, str2)` - Assert that two strings are equal

### Example Test

```c
#include "testing.h"

TEST(test_example)
{
    int value = 42;
    ASSERT(value == 42);
    ASSERT_STR_EQ("hello", "hello");
}

int main()
{
    RUN_TEST(test_example);
    return failed;
}
```

## Running Tests

### Using Make

The easiest way to run all tests is using the Makefile:

```bash
make test
```

This will run unit tests for all libraries and the main project.

### Running Individual Library Tests

Each library can be tested independently:

```bash
# Card library tests
cd server/lib/card/build
./Cardio_card_test

# Logger library tests
cd server/lib/logger/build
./Cardio_logger_test

# Pokergame library tests
cd server/lib/pokergame/build
./Cardio_pokergame_test

# Database library tests
cd server/lib/db/build
./Cardio_db_test

# Main project tests
cd server/build
./test
```

### Building Tests

Tests are automatically built when you build the libraries:

```bash
# Build all libraries (including tests)
cd server
./build_all.sh

# Or build individual library
cd server/lib/card
mkdir -p build
cd build
cmake ..
make
```

## Test Coverage

### Card Library (`server/lib/card`)

Tests cover the following functions:

#### card_init
- `test_card_init_basic` - Basic card initialization
- `test_card_init_different_suits` - All four suits
- `test_card_init_face_cards` - Jack, Queen, King, Ace

#### card_toString
- `test_card_toString_number_cards` - Number cards (2-10)
- `test_card_toString_face_cards` - Face cards (J, Q, K, A)
- `test_card_toString_all_suits` - All suits representation

#### deck_init
- `test_deck_init` - Basic deck initialization
- `test_deck_init_allocates_52_cards` - Memory allocation
- `test_deck_init_sets_topcard_to_zero` - Initial state

#### deck_fill
- `test_deck_fill_creates_52_cards` - Full deck creation
- `test_deck_fill_has_all_suits` - 13 cards per suit
- `test_deck_fill_has_all_ranks` - 4 cards per rank

#### dequeue_card
- `test_dequeue_card_removes_top_card` - Single card removal
- `test_dequeue_card_sequence` - Multiple card removal
- `test_dequeue_card_empty_deck` - Error handling

#### shuffle
- `test_shuffle_changes_order` - Randomization works
- `test_shuffle_maintains_card_count` - No cards lost
- `test_shuffle_preserves_all_cards` - All suits preserved

### Logger Library (`server/lib/logger`)

Tests cover the following functions:

#### logger
- `test_logger_creates_file` - File creation
- `test_logger_writes_correct_tag` - Tag formatting
- `test_logger_writes_correct_message` - Message content
- `test_logger_appends_to_file` - Append mode
- `test_logger_multiple_tags` - Different log levels
- `test_logger_includes_timestamp` - Timestamp formatting
- `test_logger_empty_message` - Edge case handling
- `test_logger_long_message` - Large message handling
- `test_logger_special_characters` - Special character handling

### Pokergame Library (`server/lib/pokergame`)

Tests cover:
- `test_card_toString` - Card string representation
- `test_hand_toString` - Hand display

### Database Library (`server/lib/db`)

Tests cover:
- `test_db_get_user_info` - User information retrieval
- `test_db_signup` - User registration
- `test_db_scoreboard` - Leaderboard functionality
- `test_db_friendlist` - Friend list retrieval

### Main Project (`server/test`)

Tests cover:
- Packet encoding/decoding
- Login request handling
- Signup request handling
- Response encoding
- Various protocol operations

## Writing New Tests

### Guidelines

1. **Naming Convention**: Use descriptive names starting with `test_`
   - Good: `test_card_init_basic`, `test_logger_creates_file`
   - Bad: `test1`, `mytest`

2. **Test One Thing**: Each test should verify a single behavior
   - Focus on one function or one aspect of a function
   - Use multiple tests to cover different scenarios

3. **Arrange-Act-Assert**: Follow the AAA pattern
   ```c
   TEST(test_example)
   {
       // Arrange: Set up test data
       Card card;
       
       // Act: Execute the function
       card_init(&card, SUIT_SPADE, 5);
       
       // Assert: Verify the result
       ASSERT(card.suit == SUIT_SPADE);
       ASSERT(card.rank == 5);
   }
   ```

4. **Clean Up Resources**: Always free allocated memory
   ```c
   TEST(test_with_cleanup)
   {
       char *str = card_toString(&card);
       ASSERT(str != NULL);
       free(str);  // Don't forget!
   }
   ```

5. **Test Edge Cases**: Include boundary conditions and error cases
   - Empty inputs
   - Null pointers
   - Maximum values
   - Minimum values

### Adding Tests to a Library

1. Create or update `test/unit_tests.c` in the library directory
2. Include the library header and testing.h
3. Write test functions using TEST macro
4. Add RUN_TEST calls in main()
5. Update CMakeLists.txt to build tests (see existing examples)

### Example: Adding a New Test

```c
#include "mylib.h"
#include "testing.h"

TEST(test_my_new_function)
{
    // Arrange
    int input = 10;
    
    // Act
    int result = my_new_function(input);
    
    // Assert
    ASSERT(result == 20);
}

TEST(test_my_new_function_edge_case)
{
    // Test with zero
    ASSERT(my_new_function(0) == 0);
    
    // Test with negative
    ASSERT(my_new_function(-5) == -10);
}

int main()
{
    printf("Running MyLib Tests\n");
    printf("===================\n");
    
    RUN_TEST(test_my_new_function);
    RUN_TEST(test_my_new_function_edge_case);
    
    printf("\n===================\n");
    if (failed)
    {
        printf("Some tests FAILED\n");
        return 1;
    }
    else
    {
        printf("All tests PASSED\n");
        return 0;
    }
}
```

## Continuous Integration

The project uses GitHub Actions for continuous integration. On every push and pull request, the CI pipeline:

1. Installs all dependencies (clang, cmake, PostgreSQL)
2. Builds all libraries
3. Builds the main project
4. Runs all unit tests
5. Checks code formatting with clang-format
6. Runs linting with clang-tidy

See `.github/workflows/ci.yml` for the complete configuration.

## Code Quality Tools

### Formatting

Format all C code using clang-format:

```bash
make format
```

This ensures consistent code style across the project.

### Linting

Run static analysis with clang-tidy:

```bash
make lint
```

This helps identify potential bugs and code quality issues.

## Troubleshooting

### Tests fail to build

- Ensure all dependencies are installed
- Rebuild all libraries: `cd server && ./build_all.sh`
- Check that CMakeLists.txt includes test sources

### Tests fail to run

- Check that the library is built: look for `.a` file in `lib/*/build/`
- Verify test executable exists in `lib/*/build/`
- For database tests, ensure PostgreSQL is running

### Memory leaks

- Use valgrind to detect leaks: `valgrind --leak-check=full ./test_executable`
- Ensure all allocated memory is freed in tests
- Check that test cleanup code runs even if assertions fail

## Best Practices

1. **Run tests frequently** - After every significant change
2. **Write tests first** - Test-Driven Development (TDD) when possible
3. **Keep tests fast** - Unit tests should run in milliseconds
4. **Make tests independent** - Tests should not depend on each other
5. **Use descriptive assertions** - Clear failure messages help debugging
6. **Test public API only** - Don't test internal implementation details
7. **Maintain test code** - Refactor tests as you refactor code

## Getting Help

If you encounter issues with testing:

1. Check the existing tests for examples
2. Review this documentation
3. Consult `server/lib/utils/testing.h` for available macros
4. Ask the team for guidance

## Future Improvements

Planned testing enhancements:

- [ ] Code coverage reporting
- [ ] Performance benchmarks
- [ ] Integration tests
- [ ] Mock framework for dependencies
- [ ] Automated test generation
- [ ] Test report generation
