cmake_minimum_required(VERSION 3.10)

project(DBC4C LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(dbctest dbctest.c)

target_include_directories(dbctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
    target_compile_options(dbctest PRIVATE /W4)
else()
    target_compile_options(dbctest PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_definitions(dbctest PRIVATE _POSIX_C_SOURCE=200809L)
endif()

enable_testing()
add_test(NAME dbctest COMMAND dbctest)
