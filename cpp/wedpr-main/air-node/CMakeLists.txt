aux_source_directory(. SRC_LIST)

add_executable(${AIR_BINARY_NAME} ${SRC_LIST})
target_link_libraries(${AIR_BINARY_NAME} PUBLIC ${GATEWAY_TARGET} ${RPC_TARGET} ${INIT_LIB} ${HELPER_TARGET})