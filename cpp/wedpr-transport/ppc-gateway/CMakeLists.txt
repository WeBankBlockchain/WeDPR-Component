cmake_minimum_required(VERSION 3.14)

include(Version)
project(ppc-gateway VERSION ${VERSION})

find_package(Boost COMPONENTS filesystem)

file(GLOB_RECURSE SRCS ppc-gateway/*.cpp)


add_library(${GATEWAY_TARGET} ${SRCS})
target_link_libraries(${GATEWAY_TARGET} PUBLIC ${TOOLS_TARGET} jsoncpp_static Boost::filesystem 
    ${BCOS_BOOSTSSL_TARGET} ${BCOS_UTILITIES_TARGET} 
    ${PROTOCOL_TARGET} 
    ${TARS_PROTOCOL_TARGET} ${PB_PROTOCOL_TARGET}  TBB::tbb)


# ut
if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(test)
endif ()
