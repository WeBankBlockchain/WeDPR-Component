file(GLOB_RECURSE SRCS *.cpp)
add_library(${RPC_TARGET} ${SRCS})

target_link_libraries(${RPC_TARGET} PUBLIC ${TOOLS_TARGET} ${STORAGE_TARGET} ${PROTOCOL_TARGET} jsoncpp_static ${BCOS_BOOSTSSL_TARGET})
