# export windows dll symbol
if(WIN32)
    message(STATUS "Compile on Windows")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS "ON")
endif()
add_subdirectory(ppc-crypto-c-sdk)

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(tests)
endif()
