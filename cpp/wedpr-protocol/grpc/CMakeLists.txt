# proto generation
set(PROTO_INPUT_PATH ${CMAKE_SOURCE_DIR}/wedpr-protocol/proto/pb)

file(GLOB_RECURSE MESSAGES_PROTOS "${PROTO_INPUT_PATH}/Service*.proto")

# create PROTO_OUTPUT_PATH
file(MAKE_DIRECTORY ${PROTO_OUTPUT_PATH})

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
find_program(PROTOC_BINARY protoc REQUIRED)

foreach(proto_file ${MESSAGES_PROTOS})
    get_filename_component(basename ${proto_file} NAME_WE)
    set(generated_file ${PROTO_OUTPUT_PATH}/${basename}.grpc.pb.cc)

    list(APPEND GRPC_MESSAGES_SRCS ${generated_file})
    message("Command: ${PROTOC_BINARY} --grpc_out ${PROTO_OUTPUT_PATH} -I ${PROTO_INPUT_PATH} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${proto_file}")
    add_custom_command(
        OUTPUT ${generated_file}
        COMMAND ${PROTOC_BINARY} 
        ARGS --grpc_out ${PROTO_OUTPUT_PATH} 
             -I ${PROTO_INPUT_PATH} 
             --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}" 
             ${proto_file} DEPENDS ${proto_file}
        COMMENT "Generating ${generated_file} from ${proto_file}"
    )
endforeach()

add_library(${SERVICE_CLIENT_PB_TARGET} ${GRPC_MESSAGES_SRCS})
target_link_libraries(${SERVICE_CLIENT_PB_TARGET} PUBLIC ${PB_PROTOCOL_TARGET} gRPC::grpc++_unsecure)

add_subdirectory(client)