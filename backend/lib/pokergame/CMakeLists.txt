cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall")

project(Kasino_pokergame C) # SET PROJECT NAME HERE

file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB_RECURSE TESTS "test/unit_tests.c")
list(APPEND TESTS ${SOURCES})

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..) # SET LIB DIR HERE
list(APPEND ALL_LIBS pokergame card) # APPEND LIB HERE

set(ALL_INCLUDES ${LIB_DIR}/utils ${LIB_DIR}/pokergame/include ${LIB_DIR}/card/include)

foreach(LIB IN LISTS ALL_LIBS)
    list(APPEND ALL_LIBS "${LIB_DIR}/${LIB}/include")
    list(APPEND ALL_LIBRARIES "${LIB_DIR}/${LIB}/build/libKasino_${LIB}.a")
endforeach()
list(APPEND ALL_LIBS "${LIB_DIR}/utils")

add_library(${PROJECT_NAME} ${SOURCES})

set(test Kasino_pokergame_test)
add_executable(${test} ${TESTS})

target_include_directories(${PROJECT_NAME} PUBLIC ${ALL_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ALL_LIBRARIES})

target_include_directories(${test} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${test} PRIVATE ${LIB_DIR}/card/build/libKasino_card.a)
