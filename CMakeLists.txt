cmake_minimum_required (VERSION 2.6)
project (ason_test C)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
endif()

add_library(ason ason.c)
add_executable(ason_test test.c)
target_link_libraries(ason_test ason)
