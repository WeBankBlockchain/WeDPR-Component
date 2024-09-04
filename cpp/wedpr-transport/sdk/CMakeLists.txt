cmake_minimum_required(VERSION 3.14)
project(ppc-transport-sdk VERSION ${VERSION})

file(GLOB_RECURSE SRCS *.cpp)

add_library(${WEDPR_TRANSPORT_SDK_TARGET} ${SRCS})
target_link_libraries(${WEDPR_TRANSPORT_SDK_TARGET} PUBLIC 
    ${FRONT_TARGET} ${SERVICE_CLIENT_TARGET} ${SERVICE_SERVER_TARGET})