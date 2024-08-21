project(ppc-storage VERSION ${VERSION})
add_subdirectory(src)

if (DEMO)
    add_subdirectory(demo)
endif()