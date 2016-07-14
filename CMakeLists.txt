cmake_minimum_required(VERSION 2.6)
project(libhstar C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g -std=c11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wundef -Wpointer-arith -Wcast-qual -Wcast-align -Wno-deprecated")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -fomit-frame-pointer -pipe")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")

# enable posix_memalign
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_POSIX_C_SOURCE=200112L")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DHSTAR_DEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -DNDEBUG=1")

message(STATUS "CMake using ${CMAKE_C_COMPILER_ID} C compiler")

enable_testing()
add_subdirectory(src)
