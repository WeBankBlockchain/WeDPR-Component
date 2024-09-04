set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# cmake settings
set(BINARY_NAME crypto_bench)
add_executable(${BINARY_NAME} crypto_bench.cpp)
target_link_libraries(${BINARY_NAME} ${CRYPTO_TARGET})

unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)