project(ppc-main VERSION ${VERSION})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_subdirectory(air-node)

if (BUILD_CEM)
    add_subdirectory(cem-node)
endif ()
add_subdirectory(mpc-node)

macro(compile_service SERVICE_SOURCE_PATH BINARY_NAME PKG_NAME)
    add_subdirectory(${SERVICE_SOURCE_PATH})
    add_custom_command(OUTPUT ${PKG_NAME}.tgz
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/ppc-main/${SERVICE_SOURCE_PATH}/${BINARY_NAME} ${CMAKE_BINARY_DIR}/${PKG_NAME}/${PKG_NAME}
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR} tar czfv ${PKG_NAME}.tgz ${PKG_NAME}/${PKG_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy ${PKG_NAME}.tgz tars/${PKG_NAME}.tgz
            COMMENT "Compressing ${BINARY_NAME} into ${PKG_NAME}.tgz ..."
            COMMAND ${CMAKE_COMMAND} -E rm ${PKG_NAME}.tgz
            )
    add_custom_target(${PKG_NAME}-tar DEPENDS ${PKG_NAME}.tgz ${BINARY_NAME})
    list(APPEND SERVICE_TAR_LIST ${PKG_NAME}-tar)
endmacro()

# gateway
# Note: the tar-pkg is only useful when using tars-admin
compile_service("gateway" ${GATEWAY_BINARY_NAME} ${GATEWAY_PKG_NAME})
# ppc-node
compile_service("pro-node" ${PRO_BINARY_NAME} ${PPC_NODE_PKG_NAME})

# for make tar
add_custom_target(tar DEPENDS ${SERVICE_TAR_LIST})

unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
