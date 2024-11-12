aux_source_directory(. SRC_LIST)

add_executable(${MPC_BINARY_NAME} ${SRC_LIST})
target_link_libraries(${MPC_BINARY_NAME} PUBLIC ${WEDPR_TRANSPORT_SDK_TARGET} ${MPC_TARGET} ${RPC_TARGET} ${HELPER_TARGET} TBB::tbb)
