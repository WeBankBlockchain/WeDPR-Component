file(GLOB_RECURSE SRCS *.cpp *.c)

# generate the static lib
add_library(${PPC_CRYPTO_C_SDK_STATIC_TARGET} ${SRCS})
target_link_libraries(${PPC_CRYPTO_C_SDK_STATIC_TARGET} PUBLIC ${PAILLIER_TARGET} ${IHC_TARGET} ${CRYPTO_CORE_TARGET})

# generate the shared lib
add_library(${PPC_CRYPTO_C_SDK_TARGET} SHARED ${SRCS})
target_link_libraries(${PPC_CRYPTO_C_SDK_TARGET} PUBLIC ${PAILLIER_TARGET} ${IHC_TARGET} ${CRYPTO_CORE_TARGET})