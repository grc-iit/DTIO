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
option(DTIO_ENABLE_URING "Enable URING" ON)
option(DTIO_NO_COMPILE "Do not compile DTIO" OFF)

if(DTIO_NO_COMPILE)
    install(
        FILES
        ${CMAKE_SOURCE_DIR}/.gitignore
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/.gitignore
    )
    return()
endif()

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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("IN DEBUG MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    add_compile_definitions(HERMES_LOG_VERBOSITY=10)
else()
    message("IN RELEASE MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")
    add_compile_definitions(HERMES_LOG_VERBOSITY=1)
endif()

# -----------------------------------------------------------------------------
# Find Packages
# -----------------------------------------------------------------------------
# This is for compatibility with CLANG + Spack
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

find_package(HermesShm REQUIRED)

find_package(HDF5 1.14 COMPONENTS C REQUIRED)
include_directories(${HDF5_INCLUDE_DIRS})
link_directories(${HDF5_LIBRARY_DIRS}) 
set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY})

pkg_check_modules(URING REQUIRED liburing)
include_directories(${URING_INCLUDE_DIRS})
link_directories(${URING_LIBRARY_DIRS})

if(DTIO_ENABLE_MPI)
  find_package(MPI REQUIRED)
endif()

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)

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
  target_link_libraries(${target} PUBLIC dl)
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

# ------------------------------------------------------------------------------
# Build DTIO Library
# ------------------------------------------------------------------------------

set(COMMON_SRC
    src/config_manager.cc
    src/client_metadata_manager.cc)

# Variable for setting the log level (1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE)
set(LOG_LEVEL 1 CACHE STRING "Set the log level")
add_compile_definitions(LOG_LEVEL=${LOG_LEVEL})

add_library(dtio SHARED ${DTIO_LIB_SRC}  ${COMMON_SRC})
dtio_set_libs(dtio)

add_subdirectory(benchmark)
add_subdirectory(adapter/py-bindings)
add_subdirectory(adapter/stdio)
add_subdirectory(adapter/posix)
add_subdirectory(adapter/vol)

# nanobind_add_module(dtiopy src/lib/dtio.cpp ${DTIO_LIB_SRC})
# dtio_set_libs(dtiopy)

# ------------------------------------------------------------------------------
# Install DTIO Library
# ------------------------------------------------------------------------------

install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS dtio DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(
  TARGETS dtio
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install jarvis repo
jarvis_repo_add("${CMAKE_SOURCE_DIR}/test/jarvis_dtio" "${CMAKE_SOURCE_DIR}/test/jarvis_dtio/pipelines")
