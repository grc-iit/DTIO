# Set the DTIO VOL connector name
set (TVC_NAME dtio_vol_connector)

# Build the template VOL
add_library (${TVC_NAME} SHARED H5VLDTIO.cc ${DTIO_LIB_SRC})
dtio_set_libs(${TVC_NAME})
add_dependencies(${TVC_NAME} dtio)
target_link_libraries(${TVC_NAME} PUBLIC dtio)
set_target_properties(${TVC_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${TVC_NAME} PUBLIC ${CMAKE_CXX_LINK_FLAGS})
set_target_properties (${TVC_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties (${TVC_NAME} PROPERTIES SOVERSION 1)
set_target_properties (${TVC_NAME} PROPERTIES PUBLIC_HEADER "${TVC_NAME}.h")

# Install the VOL connector
install(TARGETS ${TVC_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(
  TARGETS ${TVC_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
