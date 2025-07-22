# ------------------------------------------------------------------------------
# Set variables
# ------------------------------------------------------------------------------
set(MOD_EXPORTS ${REPO_NAMESPACE}_dtiomod_exports)

# ------------------------------------------------------------------------------
# Build dtiomod Task Library
# ------------------------------------------------------------------------------
add_chimod_runtime_lib(${REPO_NAMESPACE} dtiomod dtiomod_runtime.cc)
add_chimod_client_lib(${REPO_NAMESPACE} dtiomod dtiomod_client.cc)

# ------------------------------------------------------------------------------
# Install dtiomod Task Library
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
install(FILES dtiomod_monitor.py
        DESTINATION ${CHIMAERA_INSTALL_LIB_DIR})

# ------------------------------------------------------------------------------
# Coverage
# ------------------------------------------------------------------------------
if(CHIMAERA_ENABLE_COVERAGE)
        set_coverage_flags(dtiomod)
endif()
