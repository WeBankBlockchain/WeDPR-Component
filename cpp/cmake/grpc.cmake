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


if(NOT GRPC_CPP_REFLECTION AND TARGET gRPC::grpc++_reflection)
  get_target_property(GRPC_CPP_REFLECTION gRPC::grpc++_reflection
    IMPORTED_LOCATION_RELEASE)
  if(NOT EXISTS "${GRPC_CPP_REFLECTION}")
    get_target_property(GRPC_CPP_REFLECTION gRPC::grpc++_reflection
      IMPORTED_LOCATION_RELWITHDEBINFO)
  endif()
  if(NOT EXISTS "${GRPC_CPP_REFLECTION}")
    get_target_property(GRPC_CPP_REFLECTION gRPC::grpc++_reflection
      IMPORTED_LOCATION_MINSIZEREL)
  endif()
  if(NOT EXISTS "${GRPC_CPP_REFLECTION}")
    get_target_property(GRPC_CPP_REFLECTION gRPC::grpc++_reflection
      IMPORTED_LOCATION_DEBUG)
  endif()
  if(NOT EXISTS "${GRPC_CPP_REFLECTION}")
    get_target_property(GRPC_CPP_REFLECTION gRPC::grpc++_reflection
      IMPORTED_LOCATION_NOCONFIG)
  endif()
endif()

get_target_property(GRPC_CPP_REFLECTION_INCLUDE gRPC::grpc++_reflection INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${GRPC_CPP_REFLECTION_INCLUDE})

set(PROTOC_BINARY ${Protobuf_PROTOC_EXECUTABLE})
message("# PROTOC_BINARY: ${Protobuf_PROTOC_EXECUTABLE}")
message("# GRPC_CPP_PLUGIN: ${GRPC_CPP_PLUGIN}")
message("# GRPC_CPP_REFLECTION_INCLUDE: ${GRPC_CPP_REFLECTION_INCLUDE}")