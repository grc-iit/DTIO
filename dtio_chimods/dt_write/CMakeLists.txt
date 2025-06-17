# ------------------------------------------------------------------------------
# Build dt_write module
# ------------------------------------------------------------------------------
include_directories(include)
add_subdirectory(src)

# -----------------------------------------------------------------------------
# Install dt_write headers
# -----------------------------------------------------------------------------
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})

add_executable(dt_write_test test/test.cc)
target_link_libraries(dt_write_test example::dt_write_client)

install(
	  TARGETS
	    dt_write_test
	      EXPORT
	        ${CHIMAERA_EXPORTED_TARGETS}
		  LIBRARY DESTINATION ${CHIMAERA_INSTALL_LIB_DIR}
		    ARCHIVE DESTINATION ${CHIMAERA_INSTALL_LIB_DIR}
		      RUNTIME DESTINATION ${CHIMAERA_INSTALL_BIN_DIR}
		      )
