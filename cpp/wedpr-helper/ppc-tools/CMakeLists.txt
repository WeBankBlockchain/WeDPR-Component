aux_source_directory(src/cuckoo SRCS)
aux_source_directory(src/config SRCS)

add_library(${TOOLS_TARGET} ${SRCS})

target_link_libraries(${TOOLS_TARGET} PUBLIC ${BCOS_UTILITIES_TARGET} ${BCOS_BOOSTSSL_TARGET} ${BOOST_SERIALIZATION} ${CPU_FEATURES_LIB})

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(tests)
endif ()
