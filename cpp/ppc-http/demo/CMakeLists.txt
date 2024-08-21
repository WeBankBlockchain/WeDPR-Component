file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# cmake settings
set(BINARY_NAME http-demo)
set(BINARY_CLIENT_NAME http-client-demo)

add_executable(${BINARY_NAME} http_demo.cpp)
target_include_directories(${BINARY_NAME} PRIVATE .)
target_link_libraries(${BINARY_NAME} ${HTTP_TARGET} ${HELPER_TARGET} ${PROTOCOL_TARGET})

add_executable(${BINARY_CLIENT_NAME} http_client_demo.cpp)
target_include_directories(${BINARY_CLIENT_NAME} PRIVATE .)
target_link_libraries(${BINARY_CLIENT_NAME} ${HTTP_TARGET} ${HELPER_TARGET} ${PROTOCOL_TARGET} ${TOOLS-TARGET})

unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)