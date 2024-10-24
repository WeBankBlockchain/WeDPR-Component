set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_subdirectory(air-node)
add_subdirectory(pro-node)
add_subdirectory(gateway)

if (BUILD_CEM)
    add_subdirectory(cem-node)
endif ()
add_subdirectory(mpc-node)

unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)