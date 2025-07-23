# POSIX Interception Library
add_library(dtio_posix_interception SHARED posix_api.cc)

dtio_set_libs(dtio_posix_interception)
target_link_libraries(dtio_posix_interception PUBLIC MPI::MPI_CXX dl stdc++fs)
set_target_properties(dtio_posix_interception PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_dependencies(dtio_posix_interception dtio )

set_target_properties(dtio_posix_interception PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(dtio_posix_interception PUBLIC ${CMAKE_CXX_LINK_FLAGS})

# Install the POSIX interceptor
install(
        TARGETS
        dtio_posix_interception
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

