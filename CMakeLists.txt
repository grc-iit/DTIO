cmake_minimum_required(VERSION 3.15)
project(
  dtio
  VERSION 0.0.1
  DESCRIPTION "DTIO: A Data Task I/O system")

# -----------------------------------------------------------------------------
# Define Options
# -----------------------------------------------------------------------------
option(DTIO_ENABLE_CMAKE_DOTENV "Use cmake dotenv" ON)
option(DTIO_ENABLE_MPI "Enable MPI" ON)

# ------------------------------------------------------------------------------
# DOTENV
# ------------------------------------------------------------------------------
if(DTIO_ENABLE_CMAKE_DOTENV)
    if(EXISTS ${CMAKE_SOURCE_DIR}/.env.cmake)
        include(.env.cmake)
    endif()
endif()

# Use CPATH environment variable for include directories
if(DEFINED ENV{CPATH})
    string(REPLACE ":" ";" CPATH_LIST "$ENV{CPATH}")
    foreach(CPATH_DIR ${CPATH_LIST})
        if(EXISTS ${CPATH_DIR})
            include_directories(${CPATH_DIR})
            list(APPEND CMAKE_PREFIX_PATH ${CPATH_DIR})
        endif()
    endforeach()
endif()

# Use LIBRARY_PATH environment variable for library directories
if(DEFINED ENV{LIBRARY_PATH})
    string(REPLACE ":" ";" LIBRARY_PATH_LIST "$ENV{LIBRARY_PATH}")
    foreach(LIB_DIR ${LIBRARY_PATH_LIST})
        if(EXISTS ${LIB_DIR})
            link_directories(${LIB_DIR})
            list(APPEND CMAKE_PREFIX_PATH ${LIB_DIR})
        endif()
    endforeach()
endif()

# -----------------------------------------------------------------------------
# Compiler Optimization
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


# -----------------------------------------------------------------------------
# Find Packages
# -----------------------------------------------------------------------------
if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

find_package(HermesShm REQUIRED)

find_package(FMT REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SPDLOG REQUIRED spdlog)

find_package(HDF5 1.14 COMPONENTS C REQUIRED)
include_directories(${HDF5_INCLUDE_DIRS})
link_directories(${HDF5_LIBRARY_DIRS}) 
set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY})

pkg_check_modules(URING REQUIRED liburing)
include_directories(${URING_INCLUDE_DIRS})
link_directories(${URING_LIBRARY_DIRS})

if(DEFINED MPI)
  find_package(MPI REQUIRED)
endif()

# find_package(brahma REQUIRED)
# if (${BRAHMA_FOUND})
#     message(STATUS "[DTIO_INTERCEPTOR] found brahma headers at ${BRAHMA_INCLUDE_DIRS}")
# else ()
#     message(FATAL_ERROR "-- [DTIO_INTERCEPTOR] brahma is needed for dtio build")
# endif ()

# ------------------------------------------------------------------------------
# DTIO Library Functions
# ------------------------------------------------------------------------------

function(dtio_set_core_libs target)
  target_include_directories(${target} PUBLIC ${SPDLOG_INCLUDE_DIRS})
  target_link_libraries(${target} PUBLIC yaml-cpp ${SPDLOG_LIBRARIES})
  target_compile_definitions(${target} PUBLIC ${SPDLOG_CFLAGS_OTHER})
  target_link_directories(${target} PUBLIC ${SPDLOG_LIBRARY_DIRS}) 
  target_link_libraries(${target} PUBLIC -lm)
  target_link_libraries(${target} PUBLIC hshm::cxx)
  if(FMT_FOUND)
    target_link_libraries(${target} PUBLIC -lfmt)
  endif()
  if(DTIO_ENABLE_MPI)
    target_link_libraries(${target} PUBLIC MPI::MPI_CXX)
  endif()
  target_link_libraries(${target} PUBLIC -pthread cityhash)
  target_link_libraries(${target} PUBLIC ${HDF5_LIBRARIES})
  target_link_libraries(${target} PUBLIC ${URING_LIBRARIES})
  target_link_directories(${target} PUBLIC ${HDF5_LIBRARY_DIRS})
  target_link_directories(${target} PUBLIC ${URING_LIBRARY_DIRS})
  target_include_directories(${target} PUBLIC ${HDF5_INCLUDE_DIRS})
  target_include_directories(${target} PUBLIC ${URING_INCLUDE_DIRS})
endfunction()

function(dtio_set_libs target)
  dtio_set_core_libs(${target})
  if(NOT ${target} STREQUAL "dtio")
    message(${target})
    add_dependencies(${target} dtio)
    target_link_libraries(${target} PUBLIC dtio)
  endif()
  target_link_libraries(${target} PUBLIC example::dtiomod_client)
endfunction()

# ------------------------------------------------------------------------------
# Build DTIO Chimod
# ------------------------------------------------------------------------------

include(GNUInstallDirs)
include_directories(include)
include_directories("adapter") # <-- here 
include_directories(dtio_chimods)
add_subdirectory(dtio_chimods)

# define ones we have installed
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Build DTIO Library
# ------------------------------------------------------------------------------

set(COMMON_SRC
    src/config_manager.cpp
    src/data_manager/data_manager.cpp
    src/metadata_manager/metadata_manager.cpp
    src/external_clients/iowarp_map_impl.cpp
    src/dtio_system.cpp

    # INTERCEPTORS
    src/lib/posix.cpp 
    src/lib/mpi.cpp)

# Variable for setting the log level (1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE)
set(LOG_LEVEL 1 CACHE STRING "Set the log level")
add_compile_definitions(LOG_LEVEL=${LOG_LEVEL})

add_library(dtio SHARED ${DTIO_LIB_SRC}  ${COMMON_SRC})

dtio_set_libs(dtio)


set_target_properties(
  dtio
  PROPERTIES VERSION ${PROJECT_VERSION}
             SOVERSION 1
             PUBLIC_HEADER include/dtio.h)

set_target_properties(dtio PROPERTIES LINKER_LANGUAGE CXX)


target_include_directories(dtio PRIVATE .)

target_link_libraries(dtio PUBLIC ${CMAKE_CXX_LINK_FLAGS})


install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS dtio DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(
  TARGETS dtio
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(benchmark)
add_subdirectory(adapter/py-bindings)
add_subdirectory(adapter/stdio)
#add_subdirectory(adapter)
# add_subdirectory(hermes_shm)

# Set the DTIO VOL connector name
set (TVC_NAME dtio_vol_connector)

# Build the template VOL
add_library (${TVC_NAME} SHARED adapter/vol/H5VLDTIO.cc ${DTIO_LIB_SRC})
dtio_set_libs(${TVC_NAME})
add_dependencies(${TVC_NAME} dtio)
target_link_libraries(${TVC_NAME} PUBLIC dtio)
set_target_properties(${TVC_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${TVC_NAME} PUBLIC ${CMAKE_CXX_LINK_FLAGS})
set_target_properties (${TVC_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties (${TVC_NAME} PROPERTIES SOVERSION 1)
set_target_properties (${TVC_NAME} PROPERTIES PUBLIC_HEADER "${TVC_NAME}.h")
# add_subdirectory (adapter/vol)

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(dtiopy src/lib/dtio.cpp ${DTIO_LIB_SRC})
dtio_set_libs(dtiopy)
