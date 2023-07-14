include_directories(
	${CMAKE_SOURCE_DIR}
	${DTIO_SRC_DIR}
	${DTIO_ADAPTER_DIR}
	${DTIO_IO_CLIENT_DIR}
	${CMAKE_CURRENT_SOURCE_DIR})

# Create the metadata manager singleton + FS base class
add_library(dtio_fs_base SHARED
	${CMAKE_CURRENT_SOURCE_DIR}/filesystem.cc
	${CMAKE_CURRENT_SOURCE_DIR}/filesystem_mdm.cc
	${CMAKE_CURRENT_SOURCE_DIR}/filesystem_mdm_singleton.cc)
add_dependencies(dtio_fs_base
	dtio)
target_link_libraries(dtio_fs_base
	MPI::MPI_CXX
	dtio)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install
#-----------------------------------------------------------------------------
# install(
# 	TARGETS
# 	dtio_fs_base
# 	EXPORT
# 	${DTIO_EXPORTED_TARGETS}
# 	LIBRARY DESTINATION ${DTIO_INSTALL_LIB_DIR}
# 	ARCHIVE DESTINATION ${DTIO_INSTALL_LIB_DIR}
# 	RUNTIME DESTINATION ${DTIO_INSTALL_BIN_DIR}
# )
# install(
# 	FILES
# 	filesystem_io_client.h
# 	DESTINATION
# 	${DTIO_INSTALL_INCLUDE_DIR}
# 	COMPONENT
# 	headers
# )

#-----------------------------------------------------------------------------
# Export all exported targets to the build tree for use by parent project
#-----------------------------------------------------------------------------
# set(DTIO_EXPORTED_LIBS
# 	dtio_fs_base
# 	${DTIO_EXPORTED_LIBS})
# if(NOT DTIO_EXTERNALLY_CONFIGURED)
# 	EXPORT(
# 		TARGETS
# 		${DTIO_EXPORTED_LIBS}
# 		FILE
# 		${DTIO_EXPORTED_TARGETS}.cmake
# 	)
# endif()

#-----------------------------------------------------------------------------
# Add Target(s) to Coverage
#-----------------------------------------------------------------------------
if(DTIO_ENABLE_COVERAGE)
	set_coverage_flags(dtio_fs_base)
endif()
