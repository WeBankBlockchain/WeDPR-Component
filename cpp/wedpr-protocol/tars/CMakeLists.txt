# for tars generator
set(TARS_HEADER_DIR ${CMAKE_BINARY_DIR}/generated/tars)
find_program(TARS_TARS2CPP tars2cpp REQUIRED)

set(PROTO_INPUT_PATH ${CMAKE_SOURCE_DIR}/wedpr-protocol/proto/tars)
file(GLOB_RECURSE TARS_INPUT "${PROTO_INPUT_PATH}/*.tars")

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

file(GLOB_RECURSE SRC_LIST *.cpp)

find_package(tarscpp REQUIRED)
add_library(${TARS_PROTOCOL_TARGET} ${SRC_LIST} ${OUT_TARS_H_LIST})
target_include_directories(${TARS_PROTOCOL_TARGET} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${TARSCPP_ROOT}/include>
        $<BUILD_INTERFACE:${TARS_HEADER_DIR}>
        $<INSTALL_INTERFACE:include/wedpr-protocol/tars>)
target_link_libraries(${TARS_PROTOCOL_TARGET} PUBLIC ${BCOS_UTILITIES_TARGET} tarscpp::tarsutil)
