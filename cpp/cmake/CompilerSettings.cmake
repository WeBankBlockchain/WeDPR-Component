set(CMAKE_CXX_STANDARD 20)
set(Boost_NO_WARN_NEW_VERSIONS ON)
message(STATUS "COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"))
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
    endif()
    set(CMAKE_CXX_STANDARD 20)
    # set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "/usr/bin/time")
    # set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "/usr/bin/time")
    # Use ISO C++17 standard language.
    set(CMAKE_CXX_FLAGS "-pthread -fPIC -fexceptions")
    # set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    # Enables all the warnings about constructions that some users consider questionable,
    # and that are easy to avoid.  Also enable some extra warning flags that are not
    # enabled by -Wall.   Finally, treat at warnings-as-errors, which forces developers
    # to fix warnings as they arise, so they don't accumulate "to be fixed later".
    add_compile_options(-Werror)
    add_compile_options(-Wall)
    add_compile_options(-pedantic)
    add_compile_options(-Wextra)
    add_compile_options(-Wno-deprecated-declarations)
    add_compile_options(-Wno-unused-variable)
    add_compile_options(-Wno-unused-parameter)
    add_compile_options(-Wno-unused-function)
    add_compile_options(-Wno-unused-private-field)
    add_compile_options(-Wno-variadic-macros)
    add_compile_options(-Wno-deprecated-declarations)
    add_compile_options(-Wno-deprecated-copy)
    # add_compile_options(-Wno-missing-field-initializers)
    # Disable warnings about unknown pragmas (which is enabled by -Wall).
    add_compile_options(-Wno-unknown-pragmas)
    add_compile_options(-fno-omit-frame-pointer)
    # for boost json spirit
    add_compile_options(-DBOOST_SPIRIT_THREADSAFE)
    # for tbb, TODO: https://software.intel.com/sites/default/files/managed/b2/d2/TBBRevamp.pdf
    add_compile_options(-DTBB_SUPPRESS_DEPRECATED_MESSAGES=1)
    # build deps lib Release
    set(_only_release_configuration "-DCMAKE_BUILD_TYPE=Release")

    if(BUILD_STATIC)
        SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
        SET(BUILD_SHARED_LIBRARIES OFF)
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
        # Note: If bring the -static option, apple will fail to link
        if (NOT APPLE)
            SET(CMAKE_EXE_LINKER_FLAGS "-static")
        endif()
        # SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Bdynamic -ldl -lpthread -Wl,-Bstatic -static-libstdc++ ")
    endif ()

    if(TESTS)
        add_compile_options(-DBOOST_TEST_THREAD_SAFE)
    endif ()

    if(PROF)
    	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
		SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
    endif ()

    # Configuration-specific compiler settings.
    set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g")
    set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

    option(USE_LD_GOLD "Use GNU gold linker" ON)
    if (USE_LD_GOLD)
        execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
        if ("${LD_VERSION}" MATCHES "GNU gold")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
        endif ()
    endif ()

    # Additional GCC-specific compiler settings.
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        # Check that we've got GCC 7.0 or newer.
        set(GCC_MIN_VERSION "7.0")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
        if (NOT (GCC_VERSION VERSION_GREATER ${GCC_MIN_VERSION} OR GCC_VERSION VERSION_EQUAL ${GCC_MIN_VERSION}))
            message(FATAL_ERROR "${PROJECT_NAME} requires g++ ${GCC_MIN_VERSION} or greater. Current is ${GCC_VERSION}")
        endif ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MARCH_TYPE}")
        set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS} ${MARCH_TYPE}")

		# Strong stack protection was only added in GCC 4.9.
		# Use it if we have the option to do so.
		# See https://lwn.net/Articles/584225/
        if (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
            add_compile_options(-fstack-protector-strong)
            add_compile_options(-fstack-protector)
        endif()
    # Additional Clang-specific compiler settings.
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.0)
            set(CMAKE_CXX_FLAGS_DEBUG          "-O -g")
        endif()
        # set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}")
        add_compile_options(-fstack-protector)
        add_compile_options(-Winconsistent-missing-override)
        # Some Linux-specific Clang settings.  We don't want these for OS X.
        if ("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Linux")
            # Tell Boost that we're using Clang's libc++.   Not sure exactly why we need to do.
            add_definitions(-DBOOST_ASIO_HAS_CLANG_LIBCXX)
            # Use fancy colors in the compiler diagnostics
            add_compile_options(-fcolor-diagnostics)
        endif()
    endif()

    if (COVERAGE)
        set(TESTS ON)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        set(CMAKE_CXX_FLAGS "-g --coverage ${CMAKE_CXX_FLAGS}")
        set(CMAKE_C_FLAGS "-g --coverage ${CMAKE_C_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "--coverage ${CMAKE_SHARED_LINKER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "--coverage ${CMAKE_EXE_LINKER_FLAGS}")
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        add_compile_options(-Wno-unused-command-line-argument)
        set(CMAKE_CXX_FLAGS "-g -fprofile-arcs -ftest-coverage ${CMAKE_CXX_FLAGS}")
        set(CMAKE_C_FLAGS "-g -fprofile-arcs -ftest-coverage ${CMAKE_C_FLAGS}")
    endif()
    endif ()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
       # Only support visual studio 2017 and visual studio 2019
       set(MSVC_MIN_VERSION "1914") # VS2017 15.7, for full-ish C++17 support

       message(STATUS "Compile On Windows, MSVC_TOOLSET_VERSION: ${MSVC_TOOLSET_VERSION}")
   
       if (MSVC_TOOLSET_VERSION EQUAL 141)
           message(STATUS "Compile On Visual Studio 2017")
       elseif(MSVC_TOOLSET_VERSION EQUAL 142)
           message(STATUS "Compile On Visual Studio 2019")
       else()
           message(FATAL_ERROR "Unsupported Visual Studio, supported list: [2017, 2019]. Current MSVC_TOOLSET_VERSION: ${MSVC_TOOLSET_VERSION}")
       endif()
   
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
       add_compile_definitions(NOMINMAX)
       add_compile_options(-bigobj)
        # MSVC only support static build
        set(CMAKE_CXX_FLAGS_DEBUG "/MTd /DEBUG")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "/MT /Os")
        set(CMAKE_CXX_FLAGS_RELEASE "/MT")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /DEBUG")
        link_libraries(ws2_32 Crypt32 userenv)
else ()
    message(WARNING "Your compiler is not tested, if you run into any issues, we'd welcome any patches.")
endif ()

if (SANITIZE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=${SANITIZE}")
    if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/sanitizer-blacklist.txt")
    endif()
endif()

# rust static library linking requirements for macos
if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Security")
else()
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl")
endif()
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)
