cmake_minimum_required(VERSION 3.14)

add_subdirectory(src)

if (DEMO)
    add_subdirectory(demo)
    enable_testing()
endif ()