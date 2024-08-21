include(ExternalProject)
include(GNUInstallDirs)

find_program(CARGO_COMMAND NAMES cargo REQUIRED PATHS "${USER_HOME}\\.cargo\\bin")

if(NOT CARGO_COMMAND)
    message(FATAL_ERROR "cargo is not installed")
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/)

set(ECC_DIR "ffi/ffi_c/ffi_c_ecc_edwards25519")
ExternalProject_Add(WEDPR_CRYPTO_ECC
        PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps
        GIT_REPOSITORY https://${URL_BASE}/WeBankBlockchain/WeDPR-Lab-Crypto.git
        GIT_TAG c569186dd4bb73c5348012e15b2db18f7f20c103
        GIT_SHALLOW false
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND cd ${ECC_DIR} && ${CARGO_COMMAND} build --release
        INSTALL_COMMAND cp <SOURCE_DIR>/target/release/libffi_c_edwards25519.a ${CMAKE_SOURCE_DIR}/deps/${CMAKE_INSTALL_LIBDIR}/libffi_c_edwards25519.a
        LOG_DOWNLOAD 1
        LOG_CONFIGURE 1
        LOG_BUILD 0
        LOG_INSTALL 1
        )

ExternalProject_Get_Property(WEDPR_CRYPTO_ECC SOURCE_DIR)

set(WEDPR_LIBRARY ${CMAKE_SOURCE_DIR}/deps/${CMAKE_INSTALL_LIBDIR}/libffi_c_edwards25519.a)

add_library(wedpr_ecc INTERFACE IMPORTED)
set_property(TARGET wedpr_ecc PROPERTY INTERFACE_LINK_LIBRARIES ${WEDPR_LIBRARY})
add_dependencies(wedpr_ecc WEDPR_CRYPTO_ECC)
unset(SOURCE_DIR)
