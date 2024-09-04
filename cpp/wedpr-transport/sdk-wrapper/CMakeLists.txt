project(wedpr-transport-swig VERSION ${VERSION})

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

# Add subdirectories for each language if desired
option(BUILD_PYTHON "Build Python SWIG module" ON)
if(BUILD_PYTHON)
    include(python)
    add_subdirectory(python)
endif()

# Add subdirectories for each language if desired
option(BUILD_JAVA "Build JAVA SWIG module" ON)
if(BUILD_JAVA)
    include(java)
    add_subdirectory(java)
endif()
