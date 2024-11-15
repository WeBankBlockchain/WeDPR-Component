file(GLOB_RECURSE SRCS *.cpp)
add_library(${STORAGE_TARGET} ${SRCS})

find_package(redis++ REQUIRED)
find_package(unofficial-mysql-connector-cpp REQUIRED)

target_link_libraries(${STORAGE_TARGET} PUBLIC ${BCOS_BOOSTSSL_TARGET} redis++::redis++_static TCMalloc unofficial::mysql-connector-cpp::connector resolv ${HDFS_LIB})
