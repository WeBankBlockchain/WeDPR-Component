set(BCOS_UTILITIES_TARGET "bcos-utilities")
set(PROTOBUF_TARGET "protobuf::libprotobuf")
# ppc-crypto-core
set(CRYPTO_CORE_TARGET "ppc-crypto-core")
set(BCOS_BOOSTSSL_TARGET "bcos-boostssl")

# ppc-crypto
set(CRYPTO_TARGET "ppc-crypto")
set(PAILLIER_TARGET "ppc-paillier")
set(FAHE_TARGET "ppc-fahe")
set(IHC_TARGET "ppc-ihc")
set(CRYPTO_CODEC_TARGET "ppc-crypto-codec")

# ppc-tools
set(TOOLS_TARGET "ppc-tools")

# ppc-protocol
set(PROTOCOL_TARGET "ppc-protocol")

#  wedpr-protocol/tars
set(TARS_PROTOCOL_TARGET "wedpr-tars-protocol")

# wedpr-protocol/protobuf
set(PB_PROTOCOL_TARGET "wedpr-pb-protocol")

# wedpr-protocol/grpc/client
set(SERVICE_CLIENT_TARGET "wedpr-client")
# wedpr-protocol/grpc/server
set(SERVICE_SERVER_TARGET "wedpr-server")
# wedpr-protocol/proto generated file
set(SERVICE_PB_TARGET "service-pb")

# ppc-front
SET(FRONT_TARGET "ppc-front")

# ppc-gateway
set(GATEWAY_TARGET "ppc-gateway")

# ppc-io
set(IO_TARGET "ppc-io")

# ppc-storage
set(STORAGE_TARGET "ppc-storage")

# ppc-psi: labeled_ps
set(PSI_FRAMEWORK_TARGET "psi-framework")
set(LABELED_PSI_TARGET "ppc-labeled-psi")
set(RA2018_PSI_TARGET "ppc-ra2018-psi")
set(CM2020_PSI_TARGET "ppc-cm2020-psi")
set(ECDH_2PC_PSI_TARGET "ecdh-2pc-psi")
set(ECDH_MULTI_PSI_TARGET "ecdh-multi-psi")
# TODO: optimize here
#set(ECDH_CONN_PSI_TARGET "ecdh-conn-psi")
set(ECDH_CONN_PSI_TARGET "")

# ppc-http
set(HTTP_TARGET "ppc-http")
set(BS_ECDH_PSI_TARGET "bs-ecdh-psi")

# ppc-rpc
set(RPC_TARGET "ppc-rpc")

# libhelper
set(HELPER_TARGET "ppc-helper")

# wedpr-initializer
set(INIT_LIB "wedpr-inititializer")

# ppc-cem
set(CEM_TARGET "ppc-cem")

# ppc-mpc
set(MPC_TARGET "ppc-mpc")

# ppc-pir
set(PIR_TARGET "ppc-pir")

# the binary-name
set(GATEWAY_BINARY_NAME ppc-gateway-service)
set(GATEWAY_PKG_NAME PPCGatewayService)

# current only include the psi function
set(PRO_BINARY_NAME ppc-pro-node)
set(PPC_NODE_PKG_NAME PPCProNodeService)

set(AIR_BINARY_NAME ppc-air-node)
set(CEM_BINARY_NAME wedpr-cem)
set(MPC_BINARY_NAME wedpr-mpc)

set(HDFS_LIB libhdfs3-static)

# set cpu-info
set(CPU_FEATURES_LIB "")
if (ENABLE_CPU_FEATURES)
    set(CPU_FEATURES_LIB CpuFeatures::cpu_features)
endif ()

#====== wedpr-crypto-sdk ===========
set(PPC_CRYPTO_C_SDK_STATIC_TARGET ppc-crypto-c-sdk-static)
set(PPC_CRYPTO_C_SDK_TARGET ppc-crypto-c-sdk)

set(PPC_FRONT_C_SDK_STATIC_TARGET ppc-front-c-sdk-static)

# add suffix for arm
if(ARCH_NATIVE)
    message(STATUS "Building arm architecture, CMAKE_HOST_SYSTEM_PROCESSOR => ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    set(PPC_CRYPTO_C_SDK_STATIC_TARGET "ppc-crypto-c-sdk-aarch64")
    set(PPC_CRYPTO_C_SDK_TARGET "ppc-crypto-c-sdk-static-aarch64")
endif()
#====== wedpr-crypto-sdk ===========


#====== ppc-crypto-sdk-jni ===========
set(PPC_CRYPTO_SDK_JNI_TARGET ppc-crypto-sdk-jni)
set(PPC_CRYPTO_SDK_JNI_STATIC_TARGET ppc-crypto-sdk-jni-static)
# add suffix for arm
if(ARCH_NATIVE)
    set(PPC_CRYPTO_SDK_JNI_TARGET "ppc-crypto-sdk-jni-aarch64")
    set(PPC_CRYPTO_SDK_JNI_STATIC_TARGET "ppc-crypto-sdk-jni-static-aarch64")
endif()
#====== ppc-crypto-sdk-jni ===========

# ========== ppc-udf ===========
set(PPC_UDF_TARGET ppc-udf)
set(PPC_UDF_STATIC_TARGET ppc-udf-static)

# add suffix for arm
if(ARCH_NATIVE)
    set(PPC_UDF_TARGET ppc-udf-aarch64)
    set(PPC_UDF_STATIC_TARGET ppc-udf-static-aarch64)
endif()
# ========== ppc-udf ===========

set(BOOST_UNIT_TEST Boost::unit_test_framework)

# ==== the transport sdk===
set(WEDPR_TRANSPORT_SDK_TARGET wedpr-transport-sdk)

# ==== the swig wrapper ===== 
set(WEDPR_PYTHON_TRANSPORT "wedpr_python_transport")
set(WEDPR_PYTHON_TRANSPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/python/generated/${WEDPR_PYTHON_TRANSPORT})
set(WEDPR_PYTHON_TRANSPORT_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wedpr-transport/sdk-wrapper/python/bindings/wedpr_python_gateway_sdk/libs)

set(WEDPR_JAVA_TRANSPORT "wedpr_java_transport_jni")
set(WEDPR_JAVA_TRANSPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/java/generated/${WEDPR_JAVA_TRANSPORT})
set(WEDPR_JAVA_TRANSPORT_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wedpr-transport/sdk-wrapper/java/bindings/src/main/resources/META-INF/native)
