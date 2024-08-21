cmake_minimum_required(VERSION 3.14)

include(Version)
project(ppc-tars-protocol VERSION ${VERSION})

# for tars generator
set(TARS_HEADER_DIR ${CMAKE_BINARY_DIR}/generated/ppc-tars-protocol/tars)
find_program(TARS_TARS2CPP tars2cpp REQUIRED)

file(GLOB_RECURSE TARS_INPUT "*.tars")

# generate tars
if (TARS_INPUT)
    foreach (TARS_FILE ${TARS_INPUT})
        get_filename_component(TARS_NAME ${TARS_FILE} NAME_WE)
        get_filename_component(TARS_PATH ${TARS_FILE} PATH)
        add_custom_command(
                OUTPUT ${TARS_HEADER_DIR}/${TARS_NAME}.h
                WORKING_DIRECTORY ${TARS_PATH}
                COMMAND ${TARS_TARS2CPP} ${TARS_FILE} --unjson --without-trace --dir=${TARS_HEADER_DIR}
                COMMENT "generating ${TARS_FILE} to ${TARS_HEADER_DIR}"
                VERBATIM
        )

        list(APPEND OUT_TARS_H_LIST ${TARS_HEADER_DIR}/${TARS_NAME}.h)
    endforeach ()
endif ()

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${OUT_TARS_H_LIST}")

file(GLOB_RECURSE SRC_LIST "ppc-tars-protocol/*.cpp" "ppc-tars-protocol/*.h")

find_package(tarscpp REQUIRED)
add_library(${TARS_PROTOCOL_TARGET} ${SRC_LIST} ${OUT_TARS_H_LIST})
target_include_directories(${TARS_PROTOCOL_TARGET} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${TARSCPP_ROOT}/include>
        $<BUILD_INTERFACE:${TARS_HEADER_DIR}>
        $<INSTALL_INTERFACE:include/ppc-tars-protocol>)
target_link_libraries(${TARS_PROTOCOL_TARGET} PUBLIC ${BCOS_UTILITIES_TARGET} tarscpp::tarsservant tarscpp::tarsutil)

# ut
if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(test)
endif ()


include(GNUInstallDirs)
#install(TARGETS ${TARS_PROTOCOL_TARGET} EXPORT ppcTargets ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY "ppc-tars-protocol" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${CMAKE_BINARY_DIR}/generated/ppc-tars-protocol" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.h")