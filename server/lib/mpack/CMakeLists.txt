cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall")

project(Cardio_mpack C) # SET PROJECT NAME HERE, start with Cardio_{name}

add_library(${PROJECT_NAME} "src/mpack.c")

target_include_directories(${PROJECT_NAME} PUBLIC "include")