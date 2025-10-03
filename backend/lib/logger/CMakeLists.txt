cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall")

project(Kasino_logger C) # SET PROJECT NAME HERE

file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB_RECURSE TESTS "test/unit_tests.c")
list(APPEND TESTS ${SOURCES})

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(ALL_INCLUDES ${LIB_DIR}/utils ${LIB_DIR}/logger/include)

add_library(${PROJECT_NAME} ${SOURCES})

set(test Kasino_logger_test)
add_executable(${test} ${TESTS})

target_include_directories(${PROJECT_NAME} PUBLIC "include")

target_include_directories(${test} PUBLIC ${ALL_INCLUDES})
