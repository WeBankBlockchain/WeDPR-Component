include_directories(${CMAKE_SOURCE_DIR})


aux_source_directory(./ SRC_LIST)
add_executable(${GATEWAY_BINARY_NAME} ${SRC_LIST})

target_link_libraries(${GATEWAY_BINARY_NAME} ${GATEWAY_TARGET} ${STORAGE_TARGET} ${SERVICE_CLIENT_TARGET} ${SERVICE_SERVER_TARGET} ${HELPER_TARGET})