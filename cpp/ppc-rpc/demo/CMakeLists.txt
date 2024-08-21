file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# cmake settings
set(BINARY_NAME rpc-demo)

add_executable(${BINARY_NAME} ${SOURCES})
target_include_directories(${BINARY_NAME} PRIVATE .)
target_link_libraries(${BINARY_NAME} ${RPC_TARGET} ${HELPER_TARGET} ${PROTOCOL_TARGET} TBB::tbb)

unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)