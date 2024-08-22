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

include(IncludeDirectories)
# the target settings
include(TargetSettings)
# dependencies
include(Dependencies)

########### set the sources ########### 
set(JNI_SOURCE_PATH ppc-crypto-c-sdk/bindings/java/src/main/c)
set(SDK_SOURCE_LIST ppc-homo ppc-crypto-core ppc-crypto-c-sdk ${JNI_SOURCE_PATH})
# Note: the udf depends on mysql, not enabled in the full node mode
set(UDF_SOURCE_LIST ${SDK_SOURCE_LIST} ppc-udf)
set(ALL_SOURCE_LIST 
        ${SDK_SOURCE_LIST} ppc-crypto 
        libhelper libinitializer ppc-io ppc-protocol 
        ppc-gateway ppc-front ppc-tars-protocol 
        ppc-tools ppc-storage ppc-psi ppc-rpc 
        ppc-http ppc-mpc ppc-pir
        ${CEM_SOURCE} ppc-main)
set(CEM_SOURCE "")
if(BUILD_CEM)
    set(CEM_SOURCE "ppc-cem")
endif()

if(BUILD_ALL)
    add_sources("${ALL_SOURCE_LIST}")
elseif(BUILD_UDF)
    add_sources("${UDF_SOURCE_LIST}")
elseif(BUILD_SDK)
    add_sources("${SDK_SOURCE_LIST}")
endif()
########### set the sources end ########### 

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
endif ()

# for code coverage
if (COVERAGE)
    include(Coverage)
    config_coverage("coverage" "'/usr*' 'boost/*'")
endif ()
