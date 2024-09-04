file(GLOB_RECURSE SRCS *.cpp)
add_library(${FAHE_TARGET} ${SRCS})

find_package(OpenSSL REQUIRED)
target_link_libraries(${FAHE_TARGET} PUBLIC ${CRYPTO_CODEC_TARGET} ${BCOS_UTILITIES_TARGET} OpenSSL::Crypto ${CPU_FEATURES_LIB})