file(GLOB_RECURSE SRCS *.cpp)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${HELPER_TARGET} ${SRCS})
find_package(Boost COMPONENTS filesystem program_options)
target_link_libraries(${HELPER_TARGET} PUBLIC Boost::filesystem Boost::program_options)