project(ppc-http VERSION ${VERSION})
add_subdirectory(src)

if (DEMO)
    add_subdirectory(demo)
endif()