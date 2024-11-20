file(GLOB_RECURSE SRCS *.cpp)
add_library(${IO_TARGET} ${SRCS})
# Note: the DataBatch depends on tbb
target_link_libraries(${IO_TARGET} PUBLIC ${BCOS_UTILITIES_TARGET} ${HDFS_LIB} ${CPU_FEATURES_LIB})