file(GLOB_RECURSE SOURCES "unittests/*.cpp" "unittests/*.h")

# cmake settings
set(GATEWAY_TEST_BINARY_NAME test-ppc-gateway)

if (DEMO)
    add_subdirectory(demo)
endif ()

add_executable(${GATEWAY_TEST_BINARY_NAME} ${SOURCES})
target_include_directories(${GATEWAY_TEST_BINARY_NAME} PRIVATE .)

target_compile_options(${GATEWAY_TEST_BINARY_NAME} PRIVATE -Wno-error -Wno-unused-variable)

target_link_libraries(${GATEWAY_TEST_BINARY_NAME} ${GATEWAY_TARGET} ${FRONT_TARGET} ${PROTOCOL_TARGET} ${BOOST_UNIT_TEST})

add_test(NAME test-gateway WORKING_DIRECTORY WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${GATEWAY_TEST_BINARY_NAME})