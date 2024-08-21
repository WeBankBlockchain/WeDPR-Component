cmake_minimum_required(VERSION 3.14)

include(Version)
project(ppc-gateway VERSION ${VERSION})

find_package(Boost COMPONENTS filesystem)

file(GLOB_RECURSE SRCS ppc-gateway/*.cpp)

find_package(tarscpp REQUIRED)

add_library(${GATEWAY_TARGET} ${SRCS})
target_link_libraries(${GATEWAY_TARGET} PUBLIC ${TOOLS_TARGET} jsoncpp_static Boost::filesystem ${BCOS_BOOSTSSL_TARGET} ${BCOS_UTILITIES_TARGET} ${HTTP_TARGET} ${PROTOCOL_TARGET} ${TARS_PROTOCOL_TARGET} tarscpp::tarsservant tarscpp::tarsutil TBB::tbb)

if (APPLE)
    # target_compile_options(${GATEWAY_TARGET} PRIVATE -faligned-allocation)
endif ()

# ut
if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(test)
endif ()
