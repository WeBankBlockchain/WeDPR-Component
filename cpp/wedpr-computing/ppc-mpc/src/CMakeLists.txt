file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

add_library(${MPC_TARGET} ${SOURCES})
target_link_libraries(${MPC_TARGET} PUBLIC ${IO_TARGET} ${TOOLS_TARGET} jsoncpp_static)
