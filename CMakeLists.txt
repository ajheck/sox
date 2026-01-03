cmake_minimum_required(VERSION 3.10)
project(sox C)

# Main library
add_library(sox sox.c)

# Public include directory for consumers
target_include_directories(sox PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add tests subdirectory
add_subdirectory(test)
