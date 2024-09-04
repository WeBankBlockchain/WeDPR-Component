file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

# cmake settings
set(TEST_BINARY_NAME test-ppc-mpc)

add_executable(${TEST_BINARY_NAME} ${SOURCES})
target_include_directories(${TEST_BINARY_NAME} PRIVATE .)
target_link_libraries(${TEST_BINARY_NAME} ${MPC_TARGET} ${RPC_TARGET} ${BOOST_UNIT_TEST} TBB::tbb)
add_test(NAME test-mpc WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TEST_BINARY_NAME})