# for tars generator
set(TARS_HEADER_DIR ${CMAKE_BINARY_DIR}/generated/ppc-psi/psi/tars)
find_program(TARS_TARS2CPP tars2cpp REQUIRED)

file(GLOB_RECURSE TARS_INPUT "*.tars")
# generate tars
if(TARS_INPUT)
    foreach(TARS_FILE ${TARS_INPUT})
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
    endforeach()
endif()
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${OUT_TARS_H_LIST}")
include_directories(${TARS_HEADER_DIR})

file(GLOB_RECURSE SRCS *.cpp)
add_library(${PSI_FRAMEWORK_TARGET} ${SRCS} ${OUT_TARS_H_LIST})
target_include_directories(${PSI_FRAMEWORK_TARGET} PUBLIC
    $<BUILD_INTERFACE:${TARS_HEADER_DIR}>)
target_link_libraries(${PSI_FRAMEWORK_TARGET} PUBLIC TCMalloc ${FRONT_TARGET} ${PROTOCOL_TARGET} ${TARS_PROTOCOL_TARGET} ${IO_TARGET})
