file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

# cmake settings
set(GATEWAY_DEMO_BINARY_NAME gateway_demo)

add_executable(${GATEWAY_DEMO_BINARY_NAME} gateway_demo.cpp)

target_link_libraries(${GATEWAY_DEMO_BINARY_NAME} ${GATEWAY_TARGET} ${STORAGE_TARGET} ${FRONT_TARGET} ${PROTOCOL_TARGET} ${BOOST_UNIT_TEST})