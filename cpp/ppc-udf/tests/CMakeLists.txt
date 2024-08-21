file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

# cmake settings
set(TEST_BINARY_NAME test-ppc-udf)

add_executable(${TEST_BINARY_NAME} ${SOURCES})
target_include_directories(${TEST_BINARY_NAME} PRIVATE .)

target_link_libraries(${TEST_BINARY_NAME} ${PPC_UDF_STATIC_TARGET} ${BOOST_UNIT_TEST})
add_test(NAME test-ppc-udf WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TEST_BINARY_NAME})
