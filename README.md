# For devloper
## Prequesite

### Development Tools
- Using Linux or Unix is requirement for server
- **Compiler**: clang (not gcc)
- **Build System**: cmake >= 3.22 (see [CMake 3.22 Installation](#cmake-322-installation) below)
- **IDE**: VSCode recommended

### Development Dependencies
- `libpq-dev` (PostgreSQL client development library)
- `libcrypt-dev` (for password hashing)
- `clang-tidy` (for code linting with `make lint`)

### Runtime
- **Database**: Docker + PostgreSQL 15 (see [How to run](#how-to-run) section)
  - Default credentials: `postgres:postgres` on port `5433`
  - Configured in [docker-compose.yaml](docker-compose.yaml)

## Installation

### Linux (Ubuntu/Debian)

```bash
# Install build tools and dependencies
sudo apt-get update
sudo apt-get install -y clang cmake libpq-dev libcrypt-dev clang-tools

# Docker (optional, for PostgreSQL)
# Follow: https://docs.docker.com/engine/install/
```

### macOS

```bash
# Install with Homebrew
brew install llvm cmake libpq clang-tools
```

## How to run

### Start Database
```sh
# Start PostgreSQL with Docker
docker compose up -d

# The database will be accessible at:
# Host: localhost
# Port: 5433
# Database: cardio
# User: postgres
# Password: postgres
```

### Build and Run Server
```sh
# add excutable permission to build script
chmod u+x ./server/build_all.sh
chmod u+x ./server/clean_build.sh
 
# Build all libraries and main project
make build

# Or manually:
mkdir build
cd build
cmake ../
make

# Run the server
./server/build/Cardio_server
```

### Test the Server
```sh
# Interactive test client
./server/build/client localhost 8080

# Automated E2E test suite
./server/build/e2e_test_client localhost 8080
```

**Important:** All user passwords in the initial dataset ([data.sql](database/data.sql#L1)) are `password12345`

## Testing

The project includes comprehensive unit tests for all libraries. See [TESTING.md](TESTING.md) for detailed documentation.

### Quick Start

```sh
# Run all library tests (no database required)
make test-libs

# Run all tests including database tests (requires PostgreSQL)
make test

# Clean all build artifacts
make clean

# Format all C code
make format

# Lint all C code
make lint
```

## Workflow
- We will split project into libraries

#### Cmake
- We use `cmake`, the `CMakeLists.txt` looks like this, notice lines which I commented is what we need to define ourselves.
```sh
cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall")

project(Cardio_server C) # SET PROJECT NAME HERE, start with Cardio_{name}

file(GLOB_RECURSE SOURCES "src/*.c")

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib) # SET LIB DIR HERE, this is relative directory of "lib" folder from this Cmake dir 
list(APPEND ALL_LIBS card pokergame) # APPEND LIB HERE (directory name of lib)


foreach(LIB IN LISTS ALL_LIBS)
    list(APPEND ALL_INCLUDES "${LIB_DIR}/${LIB}/include")
    list(APPEND ALL_LIBRARIES "${LIB_DIR}/${LIB}/build/libCardio_${LIB}.a")
endforeach()

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ALL_LIBRARIES})
```
- Libraries have similar file structure:

```
server
|
|---include
|---src
|   |
|   |---main.c
|---lib
|   {lib_name}
|   |
|   |--- include (must contains {lib_name}.h for other program to include it)
|   |   |
|   |   |---{lib_name}.h (notice the same name with library)
|   |   |
|   |--- src (contains all source code, .c files)
|   |--- test (checkout utils/test.h and pokergame/test I wrote a simple template for testing)
|   |---CmakeLists.txt
|---CMakeLists.txt
|---build
```
#### First build
- First step to build, we need `build` directory in same path of your `CmakeLists.txt`, checkout the [how to run](#how-to-run)
- In the first build, you should go to every library and build it your own just like [how-to-run](#how-to-run)
- Each `lib` contains a `CMakeLists.txt` file and when you make changes in that file, go to `build/` folder of that `lib` then rebuild using `cmake ../`
- If you just make change to the source code, then running `make` in `build/` is enough
- Notice that if you make change to multiple libraries, you must rebuild every libararies in order to run it.

## Security Features

### Password Hashing (New!)
The application uses secure password hashing with SHA-512 and salt. 

**For new installations:** Passwords are automatically hashed on signup.

**For existing installations:** You must run the migration script:
```sh
cd database
DB_PASSWORD=postgres ./run_migration.sh
```

See [MIGRATION_GUIDE.md](database/MIGRATION_GUIDE.md) for detailed migration instructions.

### Enhanced Logging (New!)
The server now includes comprehensive logging with:
- **Color-coded output**: ERROR (Red), WARN (Yellow), INFO (Green), DEBUG (Cyan)
- **Function name tracking**: Every log shows which function generated it
- **Format**: `[TAG] YYYY-MM-DD HH:MM:SS [function_name] message`
- **Both file and terminal**: Logs written to `server.log` and displayed in terminal

Example log output:
```
[INFO] 2026-01-07 22:28:36 [get_listener_socket] Listener socket created successfully on 0.0.0.0:8080
[INFO] 2026-01-07 22:28:54 [accept_connection] New connection from 127.0.0.1 [fd=5]
[INFO] 2026-01-07 22:28:54 [handle_login_request] Login request from fd=5, data_len=43
[INFO] 2026-01-07 22:28:54 [handle_login_request] Login SUCCESS: user='user1' (id=1) fd=5 balance=500
```

## CMake 3.22 Installation

Ubuntu's default `apt install cmake` provides version 3.16, which is too old. Use one of these methods:

### Option 1: Pre-built Binary (Recommended - Fastest)
```bash
# Download CMake 3.22.6
wget https://github.com/Kitware/CMake/releases/download/v3.22.6/cmake-3.22.6-linux-x86_64.tar.gz

# Extract to /opt
sudo tar -xzf cmake-3.22.6-linux-x86_64.tar.gz -C /opt

# Create symlink (note the -linux-x86_64 suffix in the directory name)
sudo ln -s /opt/cmake-3.22.6-linux-x86_64/bin/cmake /usr/local/bin/cmake

# Verify
cmake --version

# Remove tar file
rm cmake-3.22.6-linux-x86_64.tar.gz
```

### Option 2: Snap
```bash
sudo snap install cmake --classic
cmake --version
```

## VSCode settings
If you're using VSCode in Linux (or MacOS), you should have `.vscode/c_cpp_properties.json` like this:
```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/lib/**/include"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/clang",
            "intelliSenseMode": "linux-clang-x64"
        }
    ],
    "version": 4
}
```