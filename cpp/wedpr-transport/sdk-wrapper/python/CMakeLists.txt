file(GLOB_RECURSE SRCS *.i)
set_source_files_properties(${SRCS} PROPERTIES CPLUSPLUS ON)

swig_add_library(
        ${WEDPR_PYTHON_TRANSPORT} 
        TYPE MODULE
        LANGUAGE python
        OUTPUT_DIR ${WEDPR_PYTHON_TRANSPORT_DIR}
        SOURCES ${SRCS}
)
 
message("#### Python3_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
target_include_directories(
        ${WEDPR_PYTHON_TRANSPORT}
        PRIVATE
        ../include
        ${Python3_INCLUDE_DIRS}
  )
set_property(TARGET ${WEDPR_PYTHON_TRANSPORT} PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON)

# note: macOS is APPLE and also UNIX !
if(APPLE)
  set_property(TARGET ${WEDPR_PYTHON_TRANSPORT} APPEND PROPERTY
    LINK_FLAGS "-flat_namespace -undefined suppress"
    )
    set_target_properties(${WEDPR_PYTHON_TRANSPORT} PROPERTIES
      SUFFIX ".so"
      INSTALL_RPATH "@loader_path;@loader_path/../../${WEDPR_PYTHON_TRANSPORT}/.libs"
    )
elseif(UNIX)
  set_target_properties(${WEDPR_PYTHON_TRANSPORT} PROPERTIES
    INSTALL_RPATH "$ORIGIN:$ORIGIN/../../${WEDPR_PYTHON_TRANSPORT}/.libs"
    )
endif()
target_link_libraries(${WEDPR_PYTHON_TRANSPORT} PRIVATE ${FRONT_TARGET})

SET(LIBRARY_OUTPUT_PATH ${WEDPR_PYTHON_TRANSPORT_DIR}/)

# Variable PYTHON_LIBRARIES can contains keyword `optimized`
# which won't be interpreted inside a generator expression.
# i.e. we can't use: $<$<PLATFORM_ID:Windows>:${PYTHON_LIBRARIES}>
# see: https://cmake.org/cmake/help/git-stage/command/target_link_libraries.html#command:target_link_libraries
if(MSVC)
  target_link_libraries(${WEDPR_PYTHON_TRANSPORT} PRIVATE ${Python3_LIBRARIES})
endif()

# Configure setup.py and copy to output directory
file(GENERATE OUTPUT ${WEDPR_PYTHON_TRANSPORT_DIR}/__init__.py CONTENT "__version__ = \"${PYTHON_TOOLKIT_VERSION}\"\n")
set(SETUP_PY_IN ${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in)
set(SETUP_PY_OUT ${WEDPR_PYTHON_TRANSPORT_DIR}/setup.py)
configure_file(${SETUP_PY_IN} ${SETUP_PY_OUT})

message(STATUS "CMAKE_INSTALL_INCLUDEDIR => ${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR => ${CMAKE_CURRENT_SOURCE_DIR}")