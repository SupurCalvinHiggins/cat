cmake_minimum_required(VERSION 3.10)

project(cat C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# set(CMAKE_C_FLAGS_DEBUG "-g -Og Wall -Wextra -Wpedantic -Wshadow -Wconversion")
# set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG Wall -Wextra -Wpedantic -Wshadow -Wconversion")

add_executable(cat cat.c)
