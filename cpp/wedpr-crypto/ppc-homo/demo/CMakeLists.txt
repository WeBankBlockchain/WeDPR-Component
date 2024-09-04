# file(GLOB SRC_LIST "*.cpp")
# file(GLOB HEADERS "*.h")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(homo_perf_demo homo_perf_demo.cpp)
# add_executable(homo_perf_demo  ${SRC_LIST} ${HEADERS})
target_link_libraries(homo_perf_demo PUBLIC ${PAILLIER_TARGET} ${FAHE_TARGET})
