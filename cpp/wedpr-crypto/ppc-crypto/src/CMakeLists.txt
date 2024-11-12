file(GLOB_RECURSE SRCS *.cpp)
add_library(${CRYPTO_TARGET} ${SRCS})


find_package(OpenSSL REQUIRED)
message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

target_link_libraries(${CRYPTO_TARGET} PUBLIC ${BCOS_UTILITIES_TARGET} ${CRYPTO_CORE_TARGET} OpenSSL::Crypto unofficial-sodium::sodium TBB::tbb ${CPU_FEATURES_LIB})

if (ENABLE_IPP_CRYPTO)
    find_package(ipp-crypto REQUIRED)
    target_link_libraries(${CRYPTO_TARGET} PUBLIC ipp-crypto::crypto_mb)
endif ()