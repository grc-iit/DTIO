# Set dtio to preload if adapter is linked.
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-DDTIO_PRELOAD -DDTIO_RPC_THALLIUM")

set(DTIO_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(DTIO_ADAPTER_DIR ${CMAKE_SOURCE_DIR}/adapter)
set(DTIO_IO_CLIENT_DIR ${CMAKE_SOURCE_DIR}/io_client)

include_directories(
        ${CMAKE_SOURCE_DIR}
        ${DTIO_SRC_DIR}
        ${DTIO_ADAPTER_DIR}
        ${DTIO_IO_CLIENT_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(filesystem)
# if(DTIO_ENABLE_POSIX_ADAPTER)
  add_subdirectory(posix)
# endif()
if(DTIO_ENABLE_STDIO_ADAPTER)
  add_subdirectory(stdio)
endif()
# if(DTIO_ENABLE_MPIIO_ADAPTER)
#   add_subdirectory(mpiio)
# endif()
# if(DTIO_ENABLE_VFD)
#   add_subdirectory(vfd)
# endif()
# if(DTIO_ENABLE_KVSTORE)
#   add_subdirectory(kvstore)
# endif()
# if(DTIO_BUILD_ADAPTER_TESTS)
#   add_subdirectory(test)
# endif()

#-----------------------------------------------------------------------------
# Specify project header files to be installed
#-----------------------------------------------------------------------------
file(GLOB_RECURSE DTIO_HEADERS "*.h")

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
# install(
#         FILES
#         ${DTIO_HEADERS}
#         DESTINATION
#         ${DTIO_INSTALL_INCLUDE_DIR}/adapter
#         COMPONENT
#         headers
# )
