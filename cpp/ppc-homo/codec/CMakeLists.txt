file(GLOB_RECURSE SRCS *.cpp)
add_library(${CRYPTO_CODEC_TARGET} ${SRCS})

find_package(OpenSSL REQUIRED)
target_link_libraries(${CRYPTO_CODEC_TARGET} PUBLIC ${BCOS_UTILITIES_TARGET} OpenSSL::Crypto ${CPU_FEATURES_LIB})