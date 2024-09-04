file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

# cmake settings
set(TEST_BINARY_NAME test-ppc-cem)

add_executable(${TEST_BINARY_NAME} ${SOURCES})
target_include_directories(${TEST_BINARY_NAME} PRIVATE .)

# target_link_libraries(${TEST_BINARY_NAME} ${CEM_TARGET} ${BCOS_UTILITIES_TARGET} jsoncpp_static ${BOOST_UNIT_TEST})
target_link_libraries(${TEST_BINARY_NAME} ${CEM_TARGET} ${RPC_TARGET} ${BOOST_UNIT_TEST})
add_test(NAME test-cem WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TEST_BINARY_NAME})
