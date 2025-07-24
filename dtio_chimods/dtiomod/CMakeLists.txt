# ------------------------------------------------------------------------------
# Build dtiomod module
# ------------------------------------------------------------------------------
include_directories(include)
add_subdirectory(src)

# -----------------------------------------------------------------------------
# Install dtiomod headers
# -----------------------------------------------------------------------------
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})

add_executable(dtiomod_test test/test.cc)
target_link_libraries(dtiomod_test example::dtiomod_client)

install(
	  TARGETS
	    dtiomod_test
	      EXPORT
	        ${CHIMAERA_EXPORTED_TARGETS}
		  LIBRARY DESTINATION ${CHIMAERA_INSTALL_LIB_DIR}
		    ARCHIVE DESTINATION ${CHIMAERA_INSTALL_LIB_DIR}
		      RUNTIME DESTINATION ${CHIMAERA_INSTALL_BIN_DIR}
		      )
