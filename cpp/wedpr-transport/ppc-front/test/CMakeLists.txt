file(GLOB_RECURSE SOURCES "unittests/*.cpp")

# cmake settings
set(FRONT_TEST_BINARY_NAME test-ppc-front)

add_executable(${FRONT_TEST_BINARY_NAME} ${SOURCES})
target_include_directories(${FRONT_TEST_BINARY_NAME} PRIVATE .)

target_compile_options(${FRONT_TEST_BINARY_NAME} PRIVATE -Wno-error -Wno-unused-variable)

target_link_libraries(${FRONT_TEST_BINARY_NAME} ${FRONT_TARGET} ${PROTOCOL_TARGET} ${BOOST_UNIT_TEST})

add_test(NAME test-front WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${FRONT_TEST_BINARY_NAME})