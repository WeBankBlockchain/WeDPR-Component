include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ppc-front)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ppc-gateway)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ppc-crypto-c-sdk)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ppc-tars-protocol)

set(VCPKG_INCLUDE_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
include_directories(${VCPKG_INCLUDE_PATH})