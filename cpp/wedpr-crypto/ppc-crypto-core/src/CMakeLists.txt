file(GLOB_RECURSE SRCS *.cpp)
add_library(${CRYPTO_CORE_TARGET} ${SRCS})

find_package(OpenSSL REQUIRED)
message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

target_link_libraries(${CRYPTO_CORE_TARGET} PUBLIC unofficial-sodium::sodium ${BCOS_UTILITIES_TARGET} OpenSSL::Crypto ${CPU_FEATURES_LIB})