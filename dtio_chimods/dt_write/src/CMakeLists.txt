# ------------------------------------------------------------------------------
# Set variables
# ------------------------------------------------------------------------------
set(MOD_EXPORTS ${REPO_NAMESPACE}_dt_write_exports)

# ------------------------------------------------------------------------------
# Build dt_write Task Library
# ------------------------------------------------------------------------------
add_chimod_runtime_lib(${REPO_NAMESPACE} dt_write dt_write_runtime.cc)
add_chimod_client_lib(${REPO_NAMESPACE} dt_write dt_write_client.cc)

# ------------------------------------------------------------------------------
# Install dt_write Task Library
# ------------------------------------------------------------------------------
install(
        TARGETS
        ${${MOD_EXPORTS}}
        EXPORT
        ${CHIMAERA_EXPORTED_TARGETS}
        LIBRARY DESTINATION ${CHIMAERA_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${CHIMAERA_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${CHIMAERA_INSTALL_BIN_DIR}
)
install(FILES dt_write_monitor.py
        DESTINATION ${CHIMAERA_INSTALL_LIB_DIR})

# ------------------------------------------------------------------------------
# Coverage
# ------------------------------------------------------------------------------
if(CHIMAERA_ENABLE_COVERAGE)
        set_coverage_flags(dt_write)
endif()
