cmake_minimum_required(VERSION 3.14)

aux_source_directory(ppc-front SRCS)


add_library(${FRONT_TARGET} ${SRCS})
target_link_libraries(${FRONT_TARGET} PUBLIC ${TARS_PROTOCOL_TARGET} TBB::tbb)

#if (TESTS)
#    enable_testing()
#    set(CTEST_OUTPUT_ON_FAILURE TRUE)
#    add_subdirectory(test)
#endif ()