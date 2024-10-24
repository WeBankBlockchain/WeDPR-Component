add_subdirectory(src)

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(tests)
endif()

if (DEMO)
    add_subdirectory(demo)
endif()