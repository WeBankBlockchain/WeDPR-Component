#------------------------------------------------------------------------------
# Top-level CMake file for WeDPR-Component
# ------------------------------------------------------------------------------
# Copyright (C) 2022 WeDPR
# SPDX-License-Identifier: Apache-2.0
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.14)

set(PROJECT_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE PATH "The cmake path for the library")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_CMAKE_DIR})

add_definitions(-w)

include(FetchContent)
if (NOT DEFINED URL_BASE)
    set(URL_BASE "github.com")
    #set(URL_BASE "github.com.cnpmjs.org")
endif ()

# export windows dll symbol
if(WIN32)
    message(STATUS "Compile on Windows")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS "ON")
endif()

# vcpkg init
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    find_package(Git REQUIRED)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- vcpkg WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

include(Version)
project(WeDPR-Component VERSION ${VERSION})

# basic settings
include(Options)
configure_project()
include(CompilerSettings)
include(BuildInfoGenerator)


find_package(OpenSSL REQUIRED)
include(InstallBcosUtilities)
find_package(Boost COMPONENTS unit_test_framework)
if(BUILD_SDK)
    ##### the sdk-dependencies #####
    # find JNI
    set(JAVA_AWT_LIBRARY NotNeeded)
    set(JAVA_JVM_LIBRARY NotNeeded)
    find_package(JNI REQUIRED)
    include_directories(${JNI_INCLUDE_DIRS})
endif()

# ipp-crypto
#if(ENABLE_IPP_CRYPTO)
#    hunter_add_package(ipp-crypto)
#endif()

if(ENABLE_CPU_FEATURES)
    find_package(CpuFeatures REQUIRED)
endif()

include(IncludeDirectories)
include(TargetSettings)

if(BUILD_SDK)
    add_subdirectory(ppc-crypto-c-sdk)
    set(JNI_SOURCE_PATH ppc-crypto-c-sdk/bindings/java/src/main/c)
    add_subdirectory(${JNI_SOURCE_PATH})
endif()
add_subdirectory(ppc-crypto-core)
add_subdirectory(ppc-homo)

# when BUILD_SDK, the following modules no need to compile
if(NOT BUILD_SDK AND NOT BUILD_UDF)
    ##### the full-dependencies #####
    find_package(TBB REQUIRED)
    find_package(jsoncpp REQUIRED)

    find_package(${BCOS_BOOSTSSL_TARGET} REQUIRED)
    # tcmalloc
    include(ProjectTCMalloc)

    find_package(SEAL REQUIRED)
    find_package(Kuku REQUIRED)

    # APSI: Note: APSI depends on seal 4.0 and Kuku 2.1
    include(ProjectAPSI)
    # Wedpr Crypto
    include(ProjectWedprCrypto)
    include(FindGSasl)
    include(Installlibhdfs3)

    add_subdirectory(ppc-crypto)
    add_subdirectory(libhelper)
    add_subdirectory(libinitializer)
    add_subdirectory(ppc-io)
    add_subdirectory(ppc-protocol)
    add_subdirectory(ppc-gateway)
    add_subdirectory(ppc-front)
    add_subdirectory(ppc-tars-protocol)
    add_subdirectory(ppc-tools)
    add_subdirectory(ppc-storage)
    add_subdirectory(ppc-psi)
    add_subdirectory(ppc-rpc)
    add_subdirectory(ppc-http)
    add_subdirectory(ppc-mpc)
    add_subdirectory(ppc-pir)
    if(BUILD_CEM)
        add_subdirectory(ppc-cem)
    endif ()
    add_subdirectory(ppc-main)
endif()
if(BUILD_UDF)
    add_subdirectory(ppc-udf)
endif()

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
endif ()

# for code coverage
if (COVERAGE)
    include(Coverage)
    config_coverage("coverage" "'/usr*' 'boost/*'")
endif ()
