cmake_minimum_required(VERSION 3.0)

project (memory_bench)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_CXX_FLAGS  "-Wall -Wextra -O3")

add_executable (proxy_libc proxy.cpp)
target_link_libraries (proxy_libc pthread)

add_executable (proxy_tc proxy.cpp)
target_link_libraries (proxy_tc pthread tcmalloc)

add_executable (proxy_je proxy.cpp)
target_link_libraries (proxy_je pthread jemalloc)

