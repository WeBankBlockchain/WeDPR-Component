cmake_minimum_required(VERSION 3.14)

project(ppctars-Gateway)

include_directories(${CMAKE_SOURCE_DIR})


aux_source_directory(./ SRC_LIST)
aux_source_directory(../../ppc-tars-service/GatewayService SRC_LIST)
add_executable(${GATEWAY_BINARY_NAME} ${SRC_LIST})

target_link_libraries(${GATEWAY_BINARY_NAME} ${GATEWAY_TARGET} ${STORAGE_TARGET} ${PROTOCOL_TARGET} ${HELPER_TARGET})