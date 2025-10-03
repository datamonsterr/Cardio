# Code Review: Cardio (C Backend for Poker Game)

## Executive Summary

The Cardio repository is a C-based backend for a poker game server. After comprehensive analysis, the codebase shows **mixed quality** - it has a good modular structure but suffers from significant security vulnerabilities, memory management issues, and incomplete implementation. The codebase is **partially maintainable** but requires substantial improvements before being production-ready.

**Overall Assessment: ⚠️ NEEDS IMPROVEMENT**

---

## 1. Code Structure & Organization

### ✅ Strengths

1. **Modular Library Architecture**
   - Clear separation of concerns with dedicated libraries:
     - `card/` - Card and deck management
     - `pokergame/` - Game logic (partially implemented)
     - `db/` - Database operations
     - `logger/` - Logging functionality
     - `mpack/` - Message packing (third-party)
   - Each library follows consistent structure: `include/`, `src/`, `test/`, `CMakeLists.txt`

2. **CMake Build System**
   - Well-structured build configuration
   - Consistent library naming convention (`Kasino_{lib}`)
   - Clear documentation in README.md

3. **Clear File Organization**
   ```
   backend/
   ├── include/        # Main headers
   ├── src/           # Main application code
   ├── lib/           # Reusable libraries
   └── test/          # Test files
   ```

### ⚠️ Weaknesses

1. **Incomplete Game Logic**
   - `backend/lib/pokergame/src/pokergame.c` - Entire main game logic is commented out (394 lines)
   - Only helper functions are implemented (hand management, player management)
   - Monte Carlo advisor code exists but main game loop is non-functional

2. **Mixed Responsibilities**
   - `main.c` contains both server initialization and request handling logic
   - Could benefit from better separation between server setup and business logic

3. **Hardcoded Configuration**
   - Database connection string hardcoded in `main.h`:
     ```c
     #define dbconninfo "dbname=cardio user=root password=1234 host=localhost port=5433"
     ```
   - No configuration file or environment variable support

---

## 2. Security Vulnerabilities

### 🔴 CRITICAL Issues

1. **SQL Injection Vulnerabilities**
   
   Multiple instances where user input is directly concatenated into SQL queries:

   **Location: `backend/lib/db/src/login.c:6`**
   ```c
   snprintf(query, sizeof(query), 
            "select user_id, password from \"User\" where username = '%s' limit 1;", 
            username);
   ```

   **Location: `backend/lib/db/src/signup.c:68`**
   ```c
   snprintf(query, sizeof(query), 
            "select user_id from \"User\" where email = '%s' OR phone = '%s' OR username = '%s';", 
            user->email, user->phone, user->username);
   ```

   **Impact**: Attackers can execute arbitrary SQL commands
   
   **Recommendation**: Use PostgreSQL parameterized queries (`PQexecParams`)

2. **Password Storage in Plaintext**
   
   **Location: `backend/lib/db/src/login.c:25`**
   ```c
   if (strcmp(db_password, password) == 0)
   ```

   Passwords are stored and compared as plaintext strings.
   
   **Recommendation**: Use bcrypt, argon2, or PBKDF2 for password hashing

3. **Hardcoded Credentials**
   
   **Location: `backend/include/main.h:22`**
   ```c
   #define dbconninfo "dbname=cardio user=root password=1234 host=localhost port=5433"
   ```
   
   **Recommendation**: Use environment variables or configuration files

### ⚠️ MEDIUM Severity Issues

1. **Buffer Overflow Potential**
   
   **Location: `backend/src/game_room.c:96`**
   ```c
   char *msg;
   sprintf(msg, "join_table: Player %s joined table %d", conn_data->username, table_id);
   ```
   
   `msg` pointer is uninitialized - this will cause undefined behavior
   
   **Recommendation**: Allocate memory or use stack buffer with `snprintf`

2. **Insufficient Input Validation**
   - Username validation only checks alphanumeric characters
   - No length limits enforced on many string inputs
   - Email/phone validation is minimal

---

## 3. Memory Management Issues

### 🔴 Memory Leaks

1. **Missing Free in Error Paths**
   
   **Location: `backend/src/handler.c:35-38`**
   ```c
   char *log_msg = malloc(100);
   sprintf(log_msg, "Handle login: Login success from socket %d\n", conn_data->fd);
   logger(MAIN_LOG, "Info", log_msg);
   return;  // log_msg is never freed
   ```

   **Location: `backend/src/handler.c:44-46`**
   Similar issue with `log_msg` malloc

2. **Database Connection Not Always Closed**
   
   **Location: `backend/src/handler.c:5-38`**
   ```c
   PGconn *conn = PQconnectdb(dbconninfo);
   // ... processing ...
   return;  // Connection not closed in success path
   ```
   
   `PQfinish(conn)` only called in failure path (line 48)

3. **Potential Double Free**
   
   **Location: `backend/lib/pokergame/src/hand.c:100-103`**
   ```c
   void hand_destroy(Hand *aHandPtr)
   {
       free(aHandPtr->cards);
       free(aHandPtr);
   }
   ```
   
   Individual cards allocated in `hand_init` are never freed before freeing the array

4. **Memory Leak in Card String Conversion**
   
   **Location: `backend/lib/card/src/card.c:12-13`**
   ```c
   char *str = (char *)malloc(sizeof(char) * 20);
   char *tmp = (char *)malloc(sizeof(char) * 5);
   ```
   
   `tmp` is allocated but never freed

### ⚠️ Unsafe Memory Operations

1. **Uninitialized Variables**
   
   **Location: `backend/lib/db/src/signup.c:30-31`**
   ```c
   bool digit;
   bool alpha;
   ```
   
   Used in line 36-44 but never initialized to false

2. **Potential Buffer Overflows**
   
   Multiple uses of `strcpy` and `strcat` without bounds checking:
   - `backend/lib/pokergame/src/hand.c:160` - `strcpy(aHandPtr->class, "Straight Flush")`
   - `backend/src/game_room.c:42` - `strncpy` used correctly but inconsistently

---

## 4. Error Handling

### ⚠️ Weaknesses

1. **Inconsistent Error Handling**
   - Some functions return -1 on error, others return 0
   - No consistent error enum or error codes
   - Many functions don't check return values

2. **Poor Error Messages**
   
   **Location: `backend/lib/db/src/signup.c:73`**
   ```c
   fprintf(stderr, "PstgreSQL error: %s\n", PQerrorMessage(conn));  // Typo: "Pstgre"
   ```

3. **Silent Failures**
   
   **Location: `backend/src/main.c:76-80`**
   ```c
   if (!conn_data || conn_data->fd <= 0)
   {
       logger(MAIN_LOG, "Error", "Invalid connection data");
       continue;  // Just continues without cleanup
   }
   ```

---

## 5. Code Quality & Maintainability

### ✅ Good Practices

1. **Consistent Naming Conventions**
   - Functions use snake_case
   - Structs use PascalCase with typedef
   - Clear prefixes (e.g., `hand_`, `deck_`, `db`)

2. **Header Guards**
   - Proper use of `#pragma once` in headers

3. **Const Correctness** (partial)
   - Some function parameters use `const` appropriately

### ⚠️ Areas for Improvement

1. **Magic Numbers**
   - Many hardcoded values without named constants:
     ```c
     char buffer[1024];  // What does 1024 represent?
     char query[256];    // Why 256?
     malloc(sizeof(char) * 20);  // Why 20?
     ```

2. **Comments**
   - Excellent block comments in `pokergame.h` explaining data structures
   - Poor inline comments explaining complex logic
   - Commented-out code should be removed (394 lines in `pokergame.c`)

3. **Code Duplication**
   - Similar pattern repeated for each packet handler
   - Could benefit from handler factory pattern

4. **Function Length**
   - `main()` function is 175 lines - too long
   - Should be broken into smaller, testable functions

---

## 6. Testing

### ⚠️ Current State

1. **Test Coverage**
   - Unit tests exist in `backend/test/unit_test.c`
   - Tests cover protocol encoding/decoding
   - No tests for game logic (because it's commented out)
   - No integration tests

2. **Test Quality**
   - Simple assertion-based testing framework
   - Tests are readable but basic
   - No test for error conditions

### Recommendations

- Add tests for all database operations
- Add tests for memory leak detection (Valgrind)
- Add integration tests for server endpoints

---

## 7. Dependencies & Build System

### ✅ Strengths

1. **CMake Configuration**
   - Well-organized CMakeLists.txt files
   - Clear dependency management between libraries
   - Supports multiple targets (server, test, client)

2. **Minimal External Dependencies**
   - PostgreSQL (libpq)
   - Standard C libraries
   - mpack (included)

### ⚠️ Weaknesses

1. **No Dependency Version Locking**
   - No specification of minimum PostgreSQL version
   - No package manager integration (vcpkg, conan)

2. **Build Scripts**
   - `build_all.sh` and `clean_build.sh` exist but not reviewed
   - No CI/CD configuration

---

## 8. Performance Considerations

### ⚠️ Potential Issues

1. **Linear Search in Table Management**
   ```c
   int find_table_by_id(TableList *table_list, int id)
   {
       for (int i = 0; i < table_list->size; i++)  // O(n) lookup
   ```
   
   Recommendation: Use hash table for O(1) lookup

2. **Inefficient String Operations**
   - Multiple `strlen()` calls in loops (e.g., `signup.c:11, 32`)
   - Should cache string length

3. **Database Connection Per Request**
   - Each request creates new PostgreSQL connection
   - Should use connection pooling

---

## 9. Concurrency & Thread Safety

### 🔴 Critical Issues

1. **No Thread Safety**
   - Global variables or shared state not protected
   - `TableList` is shared across all connections but not thread-safe

2. **Race Conditions**
   - Multiple clients can modify `table_list` simultaneously
   - No mutexes or locks observed

**Note**: Using epoll suggests single-threaded event loop, which avoids some issues, but this isn't documented

---

## 10. Documentation

### ✅ Good

1. **README.md**
   - Clear setup instructions
   - Build process documented
   - VSCode configuration provided

2. **Header Comments**
   - Good struct documentation in `pokergame.h`
   - Function prototypes listed

### ⚠️ Needs Improvement

1. **API Documentation**
   - No documentation of protocol format
   - No documentation of packet types
   - No explanation of game flow

2. **Code Comments**
   - Missing comments explaining complex algorithms
   - No explanation of Monte Carlo advisor logic

---

## Recommendations Priority List

### 🔴 Critical (Must Fix Before Production)

1. **Fix SQL injection vulnerabilities** - Use parameterized queries
2. **Implement password hashing** - Use bcrypt/argon2
3. **Fix memory leaks** - Properly free all allocated memory
4. **Remove hardcoded credentials** - Use environment variables
5. **Fix uninitialized pointer bug** - `game_room.c:96`
6. **Add thread safety** - If planning multi-threaded operation

### ⚠️ High Priority

7. **Complete game logic** - Uncomment and finish `pokergame.c`
8. **Add connection pooling** - For database
9. **Improve error handling** - Consistent error codes and messages
10. **Add input validation** - For all user inputs
11. **Close DB connections properly** - In all code paths

### 📋 Medium Priority

12. **Add comprehensive tests** - Especially for security-critical code
13. **Remove magic numbers** - Use named constants
14. **Refactor main()** - Break into smaller functions
15. **Add API documentation** - Document protocol and packet formats
16. **Performance optimization** - Hash tables, string caching

### 💡 Nice to Have

17. **Add CI/CD pipeline**
18. **Use connection pooling library**
19. **Add code coverage reports**
20. **Consider using safer string functions** - `strlcpy`, `strlcat`

---

## Conclusion

The Cardio codebase demonstrates good architectural decisions with its modular structure and clear separation of concerns. However, it suffers from critical security vulnerabilities and memory management issues that make it **unsuitable for production use** in its current state.

### Is it maintainable?

**Partially** - The modular structure makes it easier to maintain, but the incomplete game logic, security issues, and memory leaks significantly hinder maintainability.

### Is it well-structured?

**Yes** - The library-based architecture is well-structured and follows good organizational principles. The CMake build system is properly configured.

### Key Verdict:

- ✅ **Good foundation** - Solid architectural decisions
- ⚠️ **Security concerns** - Critical vulnerabilities must be addressed
- ⚠️ **Memory management** - Needs significant improvement
- ❌ **Incomplete** - Game logic is not implemented
- ⚠️ **Production readiness** - Not ready without major fixes

### Estimated Effort to Make Production-Ready:

- **Critical fixes**: 40-60 hours
- **Complete game logic**: 80-120 hours  
- **Testing & validation**: 40-60 hours
- **Documentation**: 20-30 hours

**Total**: ~180-270 hours of development work

---

## Final Score: 5/10

**Breakdown:**
- Architecture & Structure: 8/10
- Security: 2/10 🔴
- Memory Management: 4/10
- Error Handling: 5/10
- Code Quality: 6/10
- Testing: 4/10
- Documentation: 6/10
- Completeness: 3/10 (game logic incomplete)

The codebase has potential but needs significant work before being production-ready.
