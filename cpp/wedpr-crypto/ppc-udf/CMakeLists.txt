# export windows dll symbol
if(WIN32)
    message(STATUS "Compile on Windows")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS "ON")
endif()

file(GLOB_RECURSE SRCS mysql/*.cpp mysql/*.c)

if(EXISTS ${MYSQL_INCLUDE_PATH})
    include_directories(${MYSQL_INCLUDE_PATH})
else()
    find_package(MySQL REQUIRED)
    if(MYSQL_FOUND)
        include_directories(${MYSQL_INCLUDE_DIRS})
    else()
        message(FATAL "MySQL not found! Must install the mysql or specify the include path using -DMYSQL_INCLUDE_PATH")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
# generate the shared lib
add_definitions("-DHAVE_DLOPEN")
add_library(${PPC_UDF_TARGET} MODULE ${SRCS})
target_link_libraries(${PPC_UDF_TARGET} PUBLIC ${PPC_CRYPTO_C_SDK_STATIC_TARGET})

add_library(${PPC_UDF_STATIC_TARGET} ${SRCS})
target_link_libraries(${PPC_UDF_STATIC_TARGET} PUBLIC ${PPC_CRYPTO_C_SDK_STATIC_TARGET})

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(tests)
endif()
