# STDIO Interception Library
set(DTIO_STDIO_INTERCEPTION stdio_api.cc)

add_library(dtio_stdio_interception SHARED ${DTIO_STDIO_INTERCEPTION})

dtio_set_libs(dtio_stdio_interception)

add_dependencies(dtio_stdio_interception dtio)

target_link_libraries(dtio_stdio_interception PUBLIC dtio)

set_target_properties(dtio_stdio_interception PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(dtio_stdio_interception PUBLIC ${CMAKE_CXX_LINK_FLAGS})

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install
#-----------------------------------------------------------------------------
install(
        TARGETS
        dtio_stdio_interception
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

#-----------------------------------------------------------------------------
# Add Target(s) to Coverage
#-----------------------------------------------------------------------------
if(DTIO_ENABLE_COVERAGE)
    set_coverage_flags(dtio_stdio_interception) 
endif()
