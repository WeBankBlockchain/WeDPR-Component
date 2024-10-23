option(AUTO_GENERATE "Auto generate the targes" OFF)
# Note: use SWIG Version 4.2.1
if(AUTO_GENERATE)
    include(${SWIG_USE_FILE})
    # only auto-generate should find swig
    file(GLOB_RECURSE SRCS wedpr_java_transport.i)
    set_source_files_properties(${SRCS} PROPERTIES CPLUSPLUS ON)

    set(WEDPR_TRANSPORT_PACKAGE "com.webank.wedpr.sdk.jni.generated")

    set_property(SOURCE swig/wedpr_java_transport.i PROPERTY COMPILE_OPTIONS
      -package ${WEDPR_TRANSPORT_PACKAGE})

    file(MAKE_DIRECTORY ${WEDPR_JAVA_TRANSPORT_DIR})
    file(MAKE_DIRECTORY ${WEDPR_JAVA_TRANSPORT_LIB_DIR})

    swig_add_library(${WEDPR_JAVA_TRANSPORT}
      TYPE MODULE
      LANGUAGE java
      OUTPUT_DIR ${WEDPR_JAVA_TRANSPORT_DIR}
      SOURCES ${SRCS})
else()
    file(GLOB_RECURSE SRCS src/*.cxx)
    add_library(${WEDPR_JAVA_TRANSPORT} SHARED ${SRCS})
endif()  

SET(LIBRARY_OUTPUT_PATH ${WEDPR_JAVA_TRANSPORT_LIB_DIR})
message("LIBRARY_OUTPUT_PATH: ${WEDPR_JAVA_TRANSPORT_LIB_DIR}")

set_target_properties(${WEDPR_JAVA_TRANSPORT} PROPERTIES
  SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON
  POSITION_INDEPENDENT_CODE ON)

target_include_directories(${WEDPR_JAVA_TRANSPORT} PRIVATE ${JNI_INCLUDE_DIRS})

target_link_libraries(${WEDPR_JAVA_TRANSPORT} PRIVATE ${WEDPR_TRANSPORT_SDK_TARGET} ${JAVA_LIBRARIES})