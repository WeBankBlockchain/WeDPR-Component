set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# cmake settings
set(BINARY_NAME transport_sdk_demo)
add_executable(${BINARY_NAME} transport_client.cpp)
target_link_libraries(${BINARY_NAME} ${WEDPR_TRANSPORT_SDK_TARGET})

unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)