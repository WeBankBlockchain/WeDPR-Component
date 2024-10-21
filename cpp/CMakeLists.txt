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



# basic settings
include(Options)
configure_project()


# vcpkg init
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    find_package(Git REQUIRED)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- vcpkg WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

include(Version)
project(WeDPR-Component VERSION ${VERSION})

include(CompilerSettings)
include(BuildInfoGenerator)
set(PROTO_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/generated/pb)

include(IncludeDirectories)
# the target settings
include(TargetSettings)
# dependencies
include(Dependencies)

########### set the sources ########### 
set(JNI_SOURCE_PATH wedpr-crypto/sdk/bindings/java/src/main/c)
set(SDK_SOURCE_LIST wedpr-crypto/ppc-homo wedpr-crypto/ppc-crypto-core wedpr-crypto/sdk ${JNI_SOURCE_PATH})
# Note: the udf depends on mysql, not enabled in the full node mode
set(UDF_SOURCE_LIST ${SDK_SOURCE_LIST} wedpr-crypto/ppc-udf)
set(CEM_SOURCE "")
if(BUILD_CEM)
    set(CEM_SOURCE "wedpr-computing/ppc-cem")
endif()

set(TRANSPORT_SDK_SOURCE_LIST 
    wedpr-protocol 
    wedpr-transport/ppc-front 
    wedpr-transport/sdk)

set(TRANSPORT_SDK_TOOLKIT_SOURCE_LIST
    ${TRANSPORT_SDK_SOURCE_LIST} 
    wedpr-transport/sdk-wrapper)

set(ALL_SOURCE_LIST 
        ${SDK_SOURCE_LIST} 
        ${TRANSPORT_SDK_SOURCE_LIST}
        wedpr-crypto/ppc-crypto 
        wedpr-helper/libhelper wedpr-helper/ppc-tools
        wedpr-storage/ppc-io wedpr-storage/ppc-storage
        wedpr-transport/ppc-gateway 
        wedpr-transport/ppc-rpc
        wedpr-transport/ppc-http
        wedpr-computing/ppc-psi wedpr-computing/ppc-mpc wedpr-computing/ppc-pir ${CEM_SOURCE}
        wedpr-initializer wedpr-main)

if(BUILD_ALL)
    add_sources("${ALL_SOURCE_LIST}")
elseif(BUILD_UDF)
    add_sources("${UDF_SOURCE_LIST}")
elseif(BUILD_SDK)
    add_sources("${SDK_SOURCE_LIST}")
elseif(BUILD_WEDPR_TOOLKIT)
    add_sources("${TRANSPORT_SDK_TOOLKIT_SOURCE_LIST}")
endif()
########### set the sources end ########### 

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
endif ()

# for code coverage
if (COVERAGE)
    include(Coverage)
    config_coverage("coverage" "")
endif ()
