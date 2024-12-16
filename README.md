# For devloper
## Prequesite
- Using Linux or Unix is requirement for backend
- Our compiler is clang, instead of gcc
- Suggest using VSCode, or your most familiar IDE 

## How to run
```sh
mkdir build
cd build
cmake ../
make
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

## Functions in database
All functions' name are included in db.h
Functions that need intput (PGconn *conn,...), just type 'conn'
- struct Player getPlayerInfo(PGconn *conn, int player_id);
  Output is a struct type Player, containing all information of the player.
- void createPlayer(PGconn *conn, int player_id, char *fullname, char gender, char *date_of_birth, int age, char *country, char *password, float balance, int rank, char *registration_date);
  Output is none.
  This function insert a player into database. Notice this function can insert the same player many times.
- void deletePlayer(PGconn *conn, int player_id);
  Output is none.
  This function delete a player from database.
- int signup(PGconn *conn, char *username, char *password);
  Output is an integer representing if signup is successful or not (REGISTER_OK: 0, USERNAME_USED: 1, INVALID_USERNAME: 2, INVALID_PASSWORD: 3, SERVER_ERROR: -1)
  If signup is REGISTER_OK, it insert a new player to database with player_id = number of existing player + 1
- int login(PGconn *conn, char *username, char *password);
  Output is an integer representing if login is successful or not (LOGIN_OK: 0, NO_USER_FOUND: 1, SERVER_ERROR: -1)
  
