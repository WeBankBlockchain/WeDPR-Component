aux_source_directory(. SRC_LIST)

add_executable(${CEM_BINARY_NAME} ${SRC_LIST})
target_link_libraries(${CEM_BINARY_NAME} PUBLIC ${CEM_TARGET} ${RPC_TARGET} ${HELPER_TARGET})
