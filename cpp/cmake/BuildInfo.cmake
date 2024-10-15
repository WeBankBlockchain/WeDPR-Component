function(create_build_info)
    # Set build platform; to be written to BuildInfo.h
    set(PPC_BUILD_OS "${CMAKE_HOST_SYSTEM_NAME}")

    if (CMAKE_COMPILER_IS_MINGW)
        set(PPC_BUILD_COMPILER "mingw")
    elseif (CMAKE_COMPILER_IS_MSYS)
        set(PPC_BUILD_COMPILER "msys")
    elseif (CMAKE_COMPILER_IS_GNUCXX)
        set(PPC_BUILD_COMPILER "g++")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(PPC_BUILD_COMPILER "clang")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        set(PPC_BUILD_COMPILER "appleclang")
    else ()
        set(PPC_BUILD_COMPILER "unknown")
    endif ()

    set(PPC_BUILD_PLATFORM "${PPC_BUILD_OS}/${PPC_BUILD_COMPILER}")


    if (CMAKE_BUILD_TYPE)
        set(_cmake_build_type ${CMAKE_BUILD_TYPE})
    else()
        set(_cmake_build_type "${CMAKE_CFG_INTDIR}")
    endif()
    # Generate header file containing useful build information
    add_custom_target(BuildInfo.h ALL
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -DPPC_SOURCE_DIR="${PROJECT_SOURCE_DIR}/.."
        -DPPC_BUILDINFO_IN="${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/BuildInfo.h.in"
        -DPPC_DST_DIR="${PROJECT_BINARY_DIR}/include"
        -DPPC_CMAKE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/cmake"
        -DPPC_BUILD_TYPE="${_cmake_build_type}"
        -DPPC_BUILD_OS="${PPC_BUILD_OS}"
        -DPPC_BUILD_COMPILER="${PPC_BUILD_COMPILER}"
        -DPPC_BUILD_PLATFORM="${PPC_BUILD_PLATFORM}"
        -DPPC_BUILD_NUMBER="${PPC_BUILD_NUMBER}"
        -DPPC_VERSION_SUFFIX="${VERSION_SUFFIX}"
        -DPROJECT_VERSION="${PROJECT_VERSION}"
        -P "${PPC_SCRIPTS_DIR}/buildinfo.cmake"
        )
    include_directories(BEFORE ${PROJECT_BINARY_DIR})
endfunction()
