include_directories(
        ${CMAKE_SOURCE_DIR}
        ${DTIO_SRC_DIR}
        ${DTIO_ADAPTER_DIR}
        ${DTIO_IO_CLIENT_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})

# Creates the POSIX I/O client
add_library(dtio_stdio_io_client SHARED posix_io_client.cc)
add_dependencies(dtio_stdio_io_client
        dtio dtio_fs_base)
target_link_libraries(dtio_stdio_io_client
        dtio dtio_fs_base
        MPI::MPI_CXX stdc++fs dl)

# Create the POSIX interceptor
set(INTERCEPTOR_DEPS
        dtio
        dtio_stdio_io_client)
add_library(dtio_stdio SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/stdio_api.cc)
add_dependencies(dtio_stdio ${INTERCEPTOR_DEPS})
target_link_libraries(dtio_stdio ${INTERCEPTOR_DEPS})

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install
#-----------------------------------------------------------------------------
install(
        TARGETS
        dtio_stdio_io_client
        dtio_stdio
        EXPORT
        ${DTIO_EXPORTED_TARGETS}
        LIBRARY DESTINATION ${DTIO_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${DTIO_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${DTIO_INSTALL_BIN_DIR}
)

#-----------------------------------------------------------------------------
# Export all exported targets to the build tree for use by parent project
#-----------------------------------------------------------------------------
set(DTIO_EXPORTED_LIBS
        dtio_stdio_io_client
        dtio_stdio
        ${DTIO_EXPORTED_LIBS})
if(NOT DTIO_EXTERNALLY_CONFIGURED)
EXPORT (
        TARGETS
        ${DTIO_EXPORTED_LIBS}
        FILE
        ${DTIO_EXPORTED_TARGETS}.cmake
)
endif()

#-----------------------------------------------------------------------------
# Add Target(s) to Coverage
#-----------------------------------------------------------------------------
if(DTIO_ENABLE_COVERAGE)
    set_coverage_flags(dtio_stdio_io_client)
    #set_coverage_flags(dtio_stdio)
endif()
