file(GLOB_RECURSE SRCS *.cpp)
add_library(${PROTOCOL_TARGET} ${SRCS})

target_link_libraries(${PROTOCOL_TARGET} PUBLIC ${BCOS_BOOSTSSL_TARGET} ${CPU_FEATURES_LIB} jsoncpp_static)
