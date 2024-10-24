macro(default_option O DEF)
    if (DEFINED ${O})
        if (${${O}})
            set(${O} ON)
        else ()
            set(${O} OFF)
        endif ()
    else ()
        set(${O} ${DEF})
    endif ()
endmacro()
macro(add_sources source_list)
    foreach(source ${source_list})
        add_subdirectory(${source})
    endforeach()
endmacro()

# common settings
if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    message(FATAL "The ${PROJECT_NAME} does not support compiling on 32-bit systems")
endif ()

EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)

macro(configure_project)
    set(NAME ${PROJECT_NAME})

    # Default to RelWithDebInfo configuration if no configuration is explicitly specified.
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
                "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    endif ()

    default_option(BUILD_STATIC OFF)

    # unit tests
    default_option(TESTS OFF)
    # code coverage
    default_option(COVERAGE OFF)

    #debug
    default_option(DEBUG OFF)

    #cem
    default_option(BUILD_CEM OFF)

    #conn_psi
    default_option(ENABLE_CONN OFF)

    default_option(ENABLE_DEMO OFF)
    # sdk
    default_option(BUILD_ALL ON)
    default_option(BUILD_SDK OFF)
    default_option(BUILD_UDF OFF)
    default_option(BUILD_WEDPR_TOOLKIT OFF)

    # Suffix like "-rc1" e.t.c. to append to versions wherever needed.
    if (NOT DEFINED VERSION_SUFFIX)
        set(VERSION_SUFFIX "")
    endif ()

    # for boost-ssl enable/disable native
    set(ARCH_NATIVE OFF)
    if ("${ARCHITECTURE}" MATCHES "aarch64" OR "${ARCHITECTURE}" MATCHES "arm64")
        set(ARCH_NATIVE ON)
    endif ()

    set(VISIBILITY_FLAG " -fvisibility=hidden -fvisibility-inlines-hidden")
    set(PROGRAM_POSTFIX "")
    if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
        set(PROGRAM_POSTFIX ".exe")
    endif()

    set(MARCH_TYPE "-march=x86-64 -mtune=generic ${VISIBILITY_FLAG}")
    if (ARCH_NATIVE)
        set(MARCH_TYPE "-march=native -mtune=native ${VISIBILITY_FLAG}")
    endif ()

    # for enable sse4.2(hdfs used)
    set(ENABLE_SSE OFF)
    # for enable/disable ipp-crypto
    if (APPLE)
        EXECUTE_PROCESS(COMMAND sysctl -a COMMAND grep "machdep.cpu.*features" COMMAND tr -d '\n' OUTPUT_VARIABLE SUPPORTED_INSTRUCTIONS)
        message("* SUPPORTED_INSTRUCTIONS: ${SUPPORTED_INSTRUCTIONS}")
        # detect sse4.2
        if (${SUPPORTED_INSTRUCTIONS} MATCHES ".*SSE4.2.*")
            set(ENABLE_SSE ON)
        endif ()
    elseif(NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
        # detect sse4_2
        FILE(READ "/proc/cpuinfo" SUPPORTED_INSTRUCTIONS)
        message("* Linux SUPPORTED_INSTRUCTIONS: ${SUPPORTED_INSTRUCTIONS}")
        if (${SUPPORTED_INSTRUCTIONS} MATCHES ".*sse4_2.*")
            set(ENABLE_SSE ON)
        endif ()
    endif ()

    set(ENABLE_CPU_FEATURES OFF)
    # only ENABLE_CPU_FEATURES for aarch64 and x86
    if ("${ARCHITECTURE}" MATCHES "aarch64")
        add_definitions(-DARCH)
        set(ENABLE_CPU_FEATURES ON)
    endif ()

    if ("${ARCHITECTURE}" MATCHES "x86_64")
        add_definitions(-DX86)
        set(ENABLE_CPU_FEATURES ON)
    endif ()

    if (ENABLE_CPU_FEATURES)
        add_definitions(-DENABLE_CPU_FEATURES)
    endif ()

    # Enable CONN_PSI Joint Running With Ant Company  
    if (ENABLE_CONN)
        add_definitions(-DENABLE_CONN)
    endif ()

    set(ENABLE_IPP_CRYPTO OFF)
    # Note: only ENABLE_CRYPTO_MB for x86_64
    # if ("${ARCHITECTURE}" MATCHES "x86_64")
    #     set(ENABLE_IPP_CRYPTO ON)
    #     add_definitions(-DENABLE_CRYPTO_MB)
    # endif ()

    # fix the boost beast build failed for [call to 'async_teardown' is ambiguous]
    add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)

    ####### options settings ######
    if (BUILD_UDF)
        set(VISIBILITY_FLAG "")
        set(BUILD_ALL OFF)
        list(APPEND VCPKG_MANIFEST_FEATURES "sdk")
    endif()
    if (BUILD_SDK)
        set(VISIBILITY_FLAG "")
        set(BUILD_ALL OFF)
        list(APPEND VCPKG_MANIFEST_FEATURES "sdk")
    endif()
    if (BUILD_WEDPR_TOOLKIT)
        set(VISIBILITY_FLAG "")
        set(BUILD_ALL OFF)
    endif()
    if (BUILD_ALL)
        # install all dependencies
        list(APPEND VCPKG_MANIFEST_FEATURES "all")
        list(APPEND VCPKG_MANIFEST_FEATURES "sdk")
        if(ENABLE_SSE)
            # enable sse for libhdfs3
            list(APPEND VCPKG_MANIFEST_FEATURES "sse-libhdfs3")
        else()
            list(APPEND VCPKG_MANIFEST_FEATURES "default-libhdfs3")
        endif()
    endif()
    if (BUILD_WEDPR_TOOLKIT)
        # install wedpr dependencies
        list(APPEND VCPKG_MANIFEST_FEATURES "toolkit")
    endif()
    # cpp_features
    if(ENABLE_CPU_FEATURES)
        list(APPEND VCPKG_MANIFEST_FEATURES "cpufeatures")
    endif()
    ####### options settings ######
    print_config("WeDPR-Component")
endmacro()

macro(print_config NAME)
    message("")
    message("------------------------------------------------------------------------")
    message("-- Configuring ${NAME} ${PROJECT_VERSION}${VERSION_SUFFIX}")
    message("------------------------------------------------------------------------")
    message("-- CMake               Cmake version and location   ${CMAKE_VERSION} (${CMAKE_COMMAND})")
    message("-- Compiler            C++ compiler version         ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message("-- CMAKE_BUILD_TYPE    Build type                   ${CMAKE_BUILD_TYPE}")
    message("-- VCPKG_MANIFEST_FEATURES VCPKG manifest features  ${VCPKG_MANIFEST_FEATURES}")
    message("-- CMAKE_TOOLCHAIN_FILE Cmake toolchain file        ${CMAKE_TOOLCHAIN_FILE}")
    message("-- TARGET_PLATFORM     Target platform              ${CMAKE_HOST_SYSTEM_NAME} ${ARCHITECTURE}")
    message("-- BUILD_STATIC        Build static                 ${BUILD_STATIC}")
    message("-- COVERAGE            Build code coverage          ${COVERAGE}")
    message("-- TESTS               Build tests                  ${TESTS}")
    message("-- ARCH_NATIVE         Enable native code           ${ARCH_NATIVE}")
    message("-- ENABLE_CONN         Enable conn_psi ant company  ${ENABLE_CONN}")
    message("-- ENABLE_IPP_CRYPTO   Enable ipp-crypto            ${ENABLE_IPP_CRYPTO}")
    message("-- ENABLE_CPU_FEATURES Enable cpu-features          ${ENABLE_CPU_FEATURES}")
    message("-- ENABLE_SSE          Enable SSE                   ${ENABLE_SSE}")
    message("-- BUILD_CEM           Enable CEM                   ${BUILD_CEM}")
    message("-- DEMO                Enable DEMO                  ${ENABLE_DEMO}")
    message("-- BUILD_SDK           BUILD SDK                    ${BUILD_SDK}")
    message("-- BUILD_UDF           BUILD UDF                    ${BUILD_UDF}")
    message("-- BUILD_WEDPR_TOOLKIT BUILD_WEDPR_TOOLKIT          ${BUILD_WEDPR_TOOLKIT}")
    message("-- AUTO_GENERATE       AUTO_GENERATE                ${AUTO_GENERATE}")
    message("-- DEBUG                                            ${DEBUG}")
    message("------------------------------------------------------------------------")
    message("")
endmacro()