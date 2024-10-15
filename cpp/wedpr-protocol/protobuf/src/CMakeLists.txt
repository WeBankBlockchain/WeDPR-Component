# proto generation
set(PROTO_INPUT_PATH ${CMAKE_SOURCE_DIR}/wedpr-protocol/proto/pb)

file(GLOB_RECURSE MESSAGES_PROTOS "${PROTO_INPUT_PATH}/*.proto")

find_program(PROTOC_BINARY protoc REQUIRED)

# create PROTO_OUTPUT_PATH
file(MAKE_DIRECTORY ${PROTO_OUTPUT_PATH})
foreach(proto_file ${MESSAGES_PROTOS})
    get_filename_component(basename ${proto_file} NAME_WE)
    set(generated_file ${PROTO_OUTPUT_PATH}/${basename}.pb.cc)

    list(APPEND MESSAGES_SRCS ${generated_file})

    message("Command: protoc --cpp_out ${PROTO_OUTPUT_PATH} -I ${PROTO_INPUT_PATH} ${proto_file}")
    add_custom_command(
        OUTPUT ${generated_file}
        COMMAND ${PROTOC_BINARY} --cpp_out ${PROTO_OUTPUT_PATH} -I ${PROTO_INPUT_PATH} ${proto_file}
        COMMENT "Generating ${generated_file} from ${proto_file}"
        VERBATIM
    )
endforeach()

file(GLOB_RECURSE SRCS *.cpp)
add_library(${PB_PROTOCOL_TARGET} ${SRCS} ${MESSAGES_SRCS})
target_link_libraries(${PB_PROTOCOL_TARGET} PUBLIC ${BCOS_UTILITIES_TARGET} jsoncpp_static protobuf::libprotobuf ${CPU_FEATURES_LIB})