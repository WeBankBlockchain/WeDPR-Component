project(wedpr-transport-swig VERSION ${VERSION})

# Add subdirectories for each language if desired
option(BUILD_PYTHON "Build Python SWIG module" OFF)
if(BUILD_PYTHON)
    # fetch the python dependencies
    option(FETCH_PYTHON_DEPS "Install python required modules if not available" ON)
    message(STATUS "Python fetch dependencies: ${FETCH_PYTHON_DEPS}")
    include(python)
    add_subdirectory(python)
endif()

# Add subdirectories for each language if desired
option(BUILD_JAVA "Build JAVA SWIG module" ON)
if(BUILD_JAVA)
    add_subdirectory(java)
endif()
