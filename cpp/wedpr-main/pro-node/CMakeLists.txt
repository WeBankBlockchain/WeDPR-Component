include_directories(${CMAKE_SOURCE_DIR})

aux_source_directory(./ SRC_LIST)
add_executable(${PRO_BINARY_NAME} ${SRC_LIST})

target_link_libraries(${PRO_BINARY_NAME} ${RPC_TARGET} ${INIT_LIB} ${HELPER_TARGET})
