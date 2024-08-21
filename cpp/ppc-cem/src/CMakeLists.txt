file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

set(FFI_CEM_LIBRARY ${CMAKE_SOURCE_DIR}/deps/lib/libffi_c_equality.a)

add_library(${CEM_TARGET} ${SOURCES})
target_link_libraries(${CEM_TARGET} PUBLIC ${FFI_CEM_LIBRARY} ${IO_TARGET} TBB::tbb)
