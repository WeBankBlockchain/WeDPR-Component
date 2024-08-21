file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

# cmake settings
set(TEST_BINARY_NAME test-ppc-crypto-c-sdk)

add_executable(${TEST_BINARY_NAME} ${SOURCES})
target_include_directories(${TEST_BINARY_NAME} PRIVATE .)

target_link_libraries(${TEST_BINARY_NAME} ${PPC_CRYPTO_C_SDK_STATIC_TARGET} ${BOOST_UNIT_TEST})
add_test(NAME test-ppc-crypto-c-sdk WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TEST_BINARY_NAME})