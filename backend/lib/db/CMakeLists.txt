cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall")

project(Kasino_db C) # SET PROJECT NAME HERE, start with Kasino_{name}

file(GLOB_RECURSE SOURCES "src/*.c")

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib) # SET LIB DIR HERE, this is relative directory of "lib" folder from this Cmake dir 
list(APPEND ALL_LIBS db) # APPEND LIB HERE (directory name of lib)


foreach(LIB IN LISTS ALL_LIBS)
    list(APPEND ALL_INCLUDES "${LIB_DIR}/${LIB}/include")
    list(APPEND ALL_LIBRARIES "${LIB_DIR}/${LIB}/build/libKasino_${LIB}.a")
endforeach()

add_library(${PROJECT_NAME} ${SOURCES})

find_package(PostgreSQL REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC ${ALL_INCLUDES})
target_include_directories(${PROJECT_NAME} PUBLIC /usr/include/postgresql)
# target_include_directories(${PROJECT_NAME} PUBLIC /usr/include/msgpack)
target_link_libraries(${PROJECT_NAME} PUBLIC ${ALL_LIBRARIES})

# add_executable(test src/connection.c src/player.c)
add_executable(test src/connection.c src/player.c src/signup.c src/login.c)
target_link_libraries(test PostgreSQL::PostgreSQL)




