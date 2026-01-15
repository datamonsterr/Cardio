cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_C_FLAGS "-v")

project(Cardio_server C) # SET PROJECT NAME HERE, start with Cardio_{name}
set(TEST_NAME test)
set(CLIENT_NAME client)
set(E2E_TEST_NAME e2e_test_client)
set(E2E_MULTIPLAYER_TEST_NAME e2e_multiplayer_test)
set(E2E_FRIEND_TEST_NAME e2e_friend_test)
set(E2E_ACTION_SIGNED_SEQ_TEST_NAME e2e_action_signed_seq_test)
set(E2E_NEW_FEATURES_TEST_NAME e2e_new_features_test)
set(E2E_TABLE_INVITE_TEST_NAME e2e_table_invite_test)

file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test.c)
SET(CLIENT_SOURCES "test/client.c")
SET(E2E_TEST_SOURCES "test/e2e_test_client.c")
SET(E2E_MULTIPLAYER_TEST_SOURCES "test/e2e_multiplayer_test.c")
SET(E2E_FRIEND_TEST_SOURCES "test/e2e_friend_test.c")
SET(E2E_ACTION_SIGNED_SEQ_TEST_SOURCES "test/e2e_action_signed_seq_test.c")
SET(E2E_NEW_FEATURES_TEST_SOURCES "test/e2e_new_features_test.c")
SET(E2E_TABLE_INVITE_TEST_SOURCES "test/e2e_table_invite_test.c")

list(APPEND TEST_SOURCES ${SOURCES})
list(APPEND CLIENT_SOURCES ${SOURCES})
list(APPEND E2E_TEST_SOURCES ${SOURCES})
list(APPEND E2E_MULTIPLAYER_TEST_SOURCES ${SOURCES})
list(APPEND E2E_FRIEND_TEST_SOURCES ${SOURCES})
list(APPEND E2E_ACTION_SIGNED_SEQ_TEST_SOURCES ${SOURCES})
list(APPEND E2E_NEW_FEATURES_TEST_SOURCES ${SOURCES})
list(APPEND E2E_TABLE_INVITE_TEST_SOURCES ${SOURCES})

list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*main.c$")
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*main.c$")
list(FILTER E2E_TEST_SOURCES EXCLUDE REGEX ".*main.c$")
list(FILTER E2E_MULTIPLAYER_TEST_SOURCES EXCLUDE REGEX ".*main.c$")
list(FILTER E2E_FRIEND_TEST_SOURCES EXCLUDE REGEX ".*main.c$")
list(FILTER E2E_ACTION_SIGNED_SEQ_TEST_SOURCES EXCLUDE REGEX ".*main.c$")
list(FILTER E2E_NEW_FEATURES_TEST_SOURCES EXCLUDE REGEX ".*main.c$")
list(FILTER E2E_TABLE_INVITE_TEST_SOURCES EXCLUDE REGEX ".*main.c$")
# print all sources
foreach(SOURCE IN LISTS TEST_SOURCES)
    message(STATUS "SOURCE: ${SOURCE}")
endforeach()

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib) # SET LIB DIR HERE, this is relative directory of "lib" folder from this Cmake dir 
list(APPEND ALL_LIBS pokergame card mpack db logger) # APPEND LIB HERE (directory name of lib) - order matters for linking!


foreach(LIB IN LISTS ALL_LIBS)
    list(APPEND ALL_INCLUDES "${LIB_DIR}/${LIB}/include")
    list(APPEND ALL_LIBRARIES "${LIB_DIR}/${LIB}/build/libCardio_${LIB}.a")
endforeach()

list(APPEND ALL_INCLUDES "${LIB_DIR}/utils")

# add postgresql
find_package(PostgreSQL REQUIRED)
list(APPEND ALL_INCLUDES /usr/include/postgresql)
list(APPEND ALL_LIBRARIES PostgreSQL::PostgreSQL crypt)

add_executable(${PROJECT_NAME} ${SOURCES})
add_executable(${TEST_NAME} ${TEST_SOURCES})
add_executable(${CLIENT_NAME} ${CLIENT_SOURCES})
add_executable(${E2E_TEST_NAME} ${E2E_TEST_SOURCES})
add_executable(${E2E_MULTIPLAYER_TEST_NAME} ${E2E_MULTIPLAYER_TEST_SOURCES})
add_executable(${E2E_FRIEND_TEST_NAME} ${E2E_FRIEND_TEST_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "include")
target_include_directories(${PROJECT_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${ALL_LIBRARIES})

target_include_directories(${TEST_NAME} PUBLIC "include")
target_include_directories(${TEST_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${TEST_NAME} PRIVATE ${ALL_LIBRARIES})

target_include_directories(${CLIENT_NAME} PUBLIC "include")
target_include_directories(${CLIENT_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${CLIENT_NAME} PRIVATE ${ALL_LIBRARIES})

target_include_directories(${E2E_TEST_NAME} PUBLIC "include")
target_include_directories(${E2E_TEST_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${E2E_TEST_NAME} PRIVATE ${ALL_LIBRARIES})

target_include_directories(${E2E_MULTIPLAYER_TEST_NAME} PUBLIC "include")
target_include_directories(${E2E_MULTIPLAYER_TEST_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${E2E_MULTIPLAYER_TEST_NAME} PRIVATE ${ALL_LIBRARIES})

target_include_directories(${E2E_FRIEND_TEST_NAME} PUBLIC "include")
target_include_directories(${E2E_FRIEND_TEST_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${E2E_FRIEND_TEST_NAME} PRIVATE ${ALL_LIBRARIES})

add_executable(${E2E_ACTION_SIGNED_SEQ_TEST_NAME} ${E2E_ACTION_SIGNED_SEQ_TEST_SOURCES})
target_include_directories(${E2E_ACTION_SIGNED_SEQ_TEST_NAME} PUBLIC "include")
target_include_directories(${E2E_ACTION_SIGNED_SEQ_TEST_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${E2E_ACTION_SIGNED_SEQ_TEST_NAME} PRIVATE ${ALL_LIBRARIES})

add_executable(${E2E_NEW_FEATURES_TEST_NAME} ${E2E_NEW_FEATURES_TEST_SOURCES})
target_include_directories(${E2E_NEW_FEATURES_TEST_NAME} PUBLIC "include")
target_include_directories(${E2E_NEW_FEATURES_TEST_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${E2E_NEW_FEATURES_TEST_NAME} PRIVATE ${ALL_LIBRARIES})

add_executable(${E2E_TABLE_INVITE_TEST_NAME} ${E2E_TABLE_INVITE_TEST_SOURCES})
target_include_directories(${E2E_TABLE_INVITE_TEST_NAME} PUBLIC "include")
target_include_directories(${E2E_TABLE_INVITE_TEST_NAME} PUBLIC ${ALL_INCLUDES})
target_link_libraries(${E2E_TABLE_INVITE_TEST_NAME} PRIVATE ${ALL_LIBRARIES})





