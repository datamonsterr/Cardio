# For devloper
## Prequesite
- Using Linux or Unix is requirement for backend
- Our compiler is clang, instead of gcc
- Suggest using VSCode, or your most familiar IDE
- Required libraries: `libpq-dev`, `libcrypt-dev` (for password hashing) 
- cmake minimum version 3.22 (see [CMake 3.22 Installation](#cmake-322-installation) below)
- Docker (for postgres)

## How to run
```sh
# Build all libraries and main project
make build

# Or manually:
mkdir build
cd build
cmake ../
make
```

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

project(Kasino_server C) # SET PROJECT NAME HERE, start with Kasino_{name}

file(GLOB_RECURSE SOURCES "src/*.c")

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib) # SET LIB DIR HERE, this is relative directory of "lib" folder from this Cmake dir 
list(APPEND ALL_LIBS card pokergame) # APPEND LIB HERE (directory name of lib)


foreach(LIB IN LISTS ALL_LIBS)
    list(APPEND ALL_INCLUDES "${LIB_DIR}/${LIB}/include")
    list(APPEND ALL_LIBRARIES "${LIB_DIR}/${LIB}/build/libKasino_${LIB}.a")
endforeach()

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ALL_LIBRARIES})
```
- Libraries have similar file structure:

```
backend
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
The application now uses secure password hashing with SHA-512 and salt. 

**For new installations:** Passwords are automatically hashed on signup.

**For existing installations:** You must run the migration script:
```sh
cd database
./run_migration.sh
```

See `database/MIGRATION_GUIDE.md` for detailed migration instructions.

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