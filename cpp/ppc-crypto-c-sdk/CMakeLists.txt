project(ppc-crypto-c-sdk VERSION ${VERSION})

# export windows dll symbol
if(WIN32)
    message(STATUS "Compile on Windows")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS "ON")
endif()

file(GLOB_RECURSE SRCS ppc-crypto-c-sdk/*.cpp ppc-crypto-c-sdk/*.c)

# generate the static lib
add_library(${PPC_CRYPTO_C_SDK_STATIC_TARGET} ${SRCS})
target_link_libraries(${PPC_CRYPTO_C_SDK_STATIC_TARGET} PUBLIC ${PAILLIER_TARGET} ${IHC_TARGET} ${CRYPTO_CORE_TARGET})

# generate the shared lib
add_library(${PPC_CRYPTO_C_SDK_TARGET} SHARED ${SRCS})
target_link_libraries(${PPC_CRYPTO_C_SDK_TARGET} PUBLIC ${PAILLIER_TARGET} ${IHC_TARGET} ${CRYPTO_CORE_TARGET})

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(tests)
endif()