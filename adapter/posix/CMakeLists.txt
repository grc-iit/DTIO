# POSIX Interception Library
set(DTIO_POSIX_INTERCEPTION posix_api.cc)

add_library(dtio_posix_interception SHARED ${DTIO_POSIX_INTERCEPTION})

dtio_set_libs(dtio_posix_interception)

add_dependencies(dtio_posix_interception dtio)

target_link_libraries(dtio_posix_interception PUBLIC dtio)

set_target_properties(dtio_posix_interception PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(dtio_posix_interception PUBLIC ${CMAKE_CXX_LINK_FLAGS})

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install
#-----------------------------------------------------------------------------
install(
        TARGETS
        dtio_posix_interception
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
        dtio_posix_interception
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
    set_coverage_flags(dtio_posix_interception) 
endif()
