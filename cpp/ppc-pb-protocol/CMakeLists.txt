# proto generation
set(PROTO_INPUT_PATH ${CMAKE_SOURCE_DIR}/ppc-pb-protocol/src/proto)

file(GLOB_RECURSE MESSAGES_PROTOS "*.proto")

foreach(proto_file ${MESSAGES_PROTOS})
    get_filename_component(basename ${proto_file} NAME_WE)
    set(generated_file ${PROTO_OUTPUT_PATH}/${basename}.pb.cc)

    list(APPEND MESSAGES_SRCS ${generated_file})

    message("Command: protoc --cpp_out ${PROTO_OUTPUT_PATH} -I ${PROTO_INPUT_PATH} ${proto_file}")
    add_custom_command(
        OUTPUT ${generated_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTO_OUTPUT_PATH}
        COMMAND protobuf::protoc --cpp_out ${PROTO_OUTPUT_PATH} -I ${PROTO_INPUT_PATH} ${proto_file}
        COMMENT "Generating ${generated_file} from ${proto_file_abs}"
        VERBATIM
    )
endforeach()

include_directories(${PROTO_OUTPUT_PATH})

find_package(Protobuf CONFIG REQUIRED)

file(GLOB_RECURSE SRCS src/*.cpp)
add_library(${PROTO_PROTOCOL_TARGET} ${SRCS} ${MESSAGES_SRCS})
target_link_libraries(${PROTO_PROTOCOL_TARGET} PUBLIC ${BCOS_UTILITIES_TARGET})

