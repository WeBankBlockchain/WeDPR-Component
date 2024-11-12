aux_source_directory(. SRC_LIST)

add_executable(${MPC_BINARY_NAME} ${SRC_LIST})
target_link_libraries(${MPC_BINARY_NAME} PUBLIC ${MPC_TARGET} ${RPC_TARGET} ${HELPER_TARGET} ${WEDPR_TRANSPORT_SDK_TARGET} TBB::tbb)
