file(GLOB_RECURSE SRCS *.cpp)
add_library(${BS_ECDH_PSI_TARGET} ${SRCS})

target_link_libraries(${BS_ECDH_PSI_TARGET} PUBLIC ${PSI_FRAMEWORK_TARGET} wedpr_ecc TBB::tbb)
