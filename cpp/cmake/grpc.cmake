if(NOT GRPC_CPP_PLUGIN AND TARGET gRPC::grpc_cpp_plugin)
  get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin
    IMPORTED_LOCATION_RELEASE)
  if(NOT EXISTS "${GRPC_CPP_PLUGIN}")
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin
      IMPORTED_LOCATION_RELWITHDEBINFO)
  endif()
  if(NOT EXISTS "${GRPC_CPP_PLUGIN}")
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin
      IMPORTED_LOCATION_MINSIZEREL)
  endif()
  if(NOT EXISTS "${GRPC_CPP_PLUGIN}")
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin
      IMPORTED_LOCATION_DEBUG)
  endif()
  if(NOT EXISTS "${GRPC_CPP_PLUGIN}")
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin
      IMPORTED_LOCATION_NOCONFIG)
  endif()
endif()

set(PROTOC_BINARY ${Protobuf_PROTOC_EXECUTABLE})
message("# PROTOC_BINARY: ${Protobuf_PROTOC_EXECUTABLE}")
message("# GRPC_CPP_PLUGIN: ${GRPC_CPP_PLUGIN}")