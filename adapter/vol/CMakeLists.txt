# Set the DTIO VOL connector name
set (TVC_NAME dtio_vol_connector)

# Build the template VOL
add_library (${TVC_NAME} SHARED H5VLDTIO.cc ${DTIO_LIB_SRC})
set_target_properties (${TVC_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties (${TVC_NAME} PROPERTIES SOVERSION 1)
set_target_properties (${TVC_NAME} PROPERTIES PUBLIC_HEADER "H5VLDTIO.h")
# ${TVC_NAME}
