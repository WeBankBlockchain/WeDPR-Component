option(AUTO_GENERATE "Auto generate the targes" OFF)
# Note: use SWIG Version 4.2.1
if(AUTO_GENERATE)
    include(${SWIG_USE_FILE})
    file(GLOB_RECURSE SRCS *.i)
    set_source_files_properties(${SRCS} PROPERTIES CPLUSPLUS ON)

    file(MAKE_DIRECTORY ${WEDPR_PYTHON_TRANSPORT_DIR})
    file(MAKE_DIRECTORY ${WEDPR_PYTHON_TRANSPORT_LIB_DIR})

    swig_add_library(
            ${WEDPR_PYTHON_TRANSPORT} 
            TYPE MODULE
            LANGUAGE python
            OUTPUT_DIR ${WEDPR_PYTHON_TRANSPORT_DIR}
            SOURCES ${SRCS}
    )
    set_property(TARGET ${WEDPR_PYTHON_TRANSPORT} PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON)
else()
    file(GLOB_RECURSE SRCS src/*.cxx)
    add_library(${WEDPR_PYTHON_TRANSPORT} SHARED ${SRCS})
endif()  

target_include_directories(${WEDPR_PYTHON_TRANSPORT} PRIVATE ${Python_INCLUDE_DIRS})

# note: macOS is APPLE and also UNIX !
if(APPLE)
  set_property(TARGET ${WEDPR_PYTHON_TRANSPORT} APPEND PROPERTY
    LINK_FLAGS "-flat_namespace -undefined suppress"
    )
  set_target_properties(${WEDPR_PYTHON_TRANSPORT} PROPERTIES
    INSTALL_RPATH "@loader_path;@loader_path/../../${WEDPR_PYTHON_TRANSPORT}/.libs"
    )
elseif(UNIX)
  set_target_properties(${WEDPR_PYTHON_TRANSPORT} PROPERTIES
    INSTALL_RPATH "$ORIGIN:$ORIGIN/../../${WEDPR_PYTHON_TRANSPORT}/.libs"
    )
endif()
# Variable PYTHON_LIBRARIES can contains keyword `optimized`
# which won't be interpreted inside a generator expression.
# i.e. we can't use: $<$<PLATFORM_ID:Windows>:${PYTHON_LIBRARIES}>
# see: https://cmake.org/cmake/help/git-stage/command/target_link_libraries.html#command:target_link_libraries
if(MSVC)
  target_link_libraries(${WEDPR_PYTHON_TRANSPORT} PRIVATE ${WEDPR_TRANSPORT_SDK_TARGET} ${Python3_LIBRARIES})
else()
  target_link_libraries(${WEDPR_PYTHON_TRANSPORT} PRIVATE ${WEDPR_TRANSPORT_SDK_TARGET})
endif()
SET(LIBRARY_OUTPUT_PATH ${WEDPR_PYTHON_TRANSPORT_LIB_DIR})

message(STATUS "CMAKE_INSTALL_INCLUDEDIR => ${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR => ${CMAKE_CURRENT_SOURCE_DIR}")