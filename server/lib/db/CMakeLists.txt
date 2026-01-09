cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall")

project(Cardio_db C) # SET PROJECT NAME HERE, start with Cardio_{name}

file(GLOB_RECURSE SOURCES "src/*.c")
list(FILTER SOURCES EXCLUDE REGEX ".*main.c$")

file(GLOB_RECURSE TESTS "test/unit_test.c")
list(APPEND TESTS ${SOURCES})

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..) # SET LIB DIR HERE, this is relative directory of "lib" folder from this Cmake dir 

set(ALL_INCLUDES ${LIB_DIR}/utils ${LIB_DIR}/db/include ${LIB_DIR}/logger/include /usr/include/postgresql) # SET ALL INCLUDES HERE

add_library(${PROJECT_NAME} ${SOURCES})

set(test Cardio_db_test)
add_executable(${test} ${TESTS})

find_package(PostgreSQL REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${PROJECT_NAME} PostgreSQL::PostgreSQL crypt ${LIB_DIR}/logger/build/libCardio_logger.a)

target_include_directories(${test} PUBLIC ${ALL_INCLUDES})
target_include_directories(${test} PUBLIC /usr/include/postgresql)
target_link_libraries(${test} PostgreSQL::PostgreSQL crypt ${LIB_DIR}/logger/build/libCardio_logger.a)




