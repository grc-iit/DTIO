# ------------------------------------------------------------------------------
# Build dtio module
# ------------------------------------------------------------------------------
include_directories(include)
add_subdirectory(src)

# -----------------------------------------------------------------------------
# Install dtio headers
# -----------------------------------------------------------------------------
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})

add_executable(dtio_test test/test.cc)
target_link_libraries(dtio_test example::dtio_client)

install(
	  TARGETS
	    dtio_test
	      EXPORT
	        ${CHIMAERA_EXPORTED_TARGETS}
		  LIBRARY DESTINATION ${CHIMAERA_INSTALL_LIB_DIR}
		    ARCHIVE DESTINATION ${CHIMAERA_INSTALL_LIB_DIR}
		      RUNTIME DESTINATION ${CHIMAERA_INSTALL_BIN_DIR}
		      )
