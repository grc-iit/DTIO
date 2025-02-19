cmake_minimum_required(VERSION 3.15)
project(
  dtio
  VERSION 0.0.1
  DESCRIPTION "DTIO: A Data Task I/O system")

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)
include(GNUInstallDirs)
include_directories(include)
# include_directories("hermes_shm/include")
include_directories("adapter") # <-- here 

# include_directories("")
include_directories(${INCLUDE_PATH})
link_libraries(${LIBRARY_PATH})
# define ones we have installed
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MEMCACHED false)
set(MPI true)
set(HCL true)
set(ROCKS false)
set(CMAKE_CXX_FLAGS "-g")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-usage")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERMDM")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERTS")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERTB")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERDM")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERW")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMER")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFTIMER")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOLLECT")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOVERHEADS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERBASE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTABIOS")

set(COMMON_SRC
    src/common/solver/default_solver.cpp
    src/common/config_manager.cpp
    src/common/solver/dp_solver.cpp
    src/common/solver/random_solver.cpp
    src/common/solver/round_robin_solver.cpp
    src/common/solver/knapsack.cpp
    # src/common/timer.cpp
    src/common/data_manager/data_manager.cpp
    src/common/metadata_manager/metadata_manager.cpp
    src/dtio_system.cpp
    src/common/client_interface/distributed_hashmap.cpp
    src/common/client_interface/distributed_queue.cpp
    src/common/external_clients/rocksdb_impl.cpp
    # src/common/external_clients/memcached_impl.cpp
    src/common/external_clients/hcl_queue_impl.cpp
    src/common/external_clients/hcl_map_impl.cpp
    src/common/task_builder/task_builder.cpp
    src/common/task_builder/default_builder.cpp
    src/common/task_builder/aggregating_builder.cpp)
set(DTIO_LIB_SRC src/lib/posix.cpp src/lib/stdio.cpp src/lib/hdf5.cpp src/lib/mpi.cpp src/common/threadPool.cpp) # src/lib/stdio.cpp
set(DTIO_POSIX_INTERCEPTION adapter/posix/posix_api.cc)
set(DTIO_STDIO_INTERCEPTION adapter/stdio/stdio_api.cc)
# set(DTIO_BRAHMA_INTERCEPTION adapter/brahma-interceptor/posix_api.cc)
set(DTIO_CLIENT_SRC src/client/main.cpp src/client/client.cpp
                      src/common/threadPool.cpp)
set(DTIO_SERVER_SRC src/system_manager/main.cpp
                      src/system_manager/system_manager_service.cpp)
set(DTIO_TS_SRC
    src/task_scheduler/main.cpp src/task_scheduler/task_scheduler.cpp
    src/task_scheduler/task_scheduler.h)
set(DTIO_WM_SRC src/worker_manager/main.cpp
                  src/worker_manager/worker_manager_service.cpp)
set(DTIO_WS_SRC
    src/worker/api/posix_client.cpp src/worker/api/stdio_client.cpp src/worker/api/uring_client.cpp src/worker/api/hdf5_client.cpp src/worker/api/multi_client.cpp src/worker/program_repo/statistics.cpp
    src/worker/main.cpp src/worker/worker.cpp)
set(DTIO_HM_SRC src/common/external_clients/hcl_manager/main.cpp
    src/common/external_clients/hcl_map_impl.cpp src/common/external_clients/hcl_queue_impl.cpp)
    

if(DEFINED MPI)
  find_package(MPI REQUIRED)
  set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
  set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
  include_directories(${MPI_INCLUDE_PATH})
endif()
if(MEMCACHED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lmemcached -DMEMCACHED_P")
endif()
if(DEFINED HCL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lhcl -lmercury -lmargo -labt -lcpp-logger -lhdf5 -DHCL_P")
endif()
if(ROCKS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DROCKS_P")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# find_package(brahma REQUIRED)
# if (${BRAHMA_FOUND})
#     message(STATUS "[DTIO_INTERCEPTOR] found brahma headers at ${BRAHMA_INCLUDE_DIRS}")
#     include_directories(${BRAHMA_INCLUDE_DIRS})
#     set(DEPENDENCY_LIB ${DEPENDENCY_LIB} ${BRAHMA_LIBRARIES})
#     link_directories("${HOME}/root/lib/")
#     message(STATUS "[DTIO_INTERCEPTOR] found brahma lib at ${BRAHMA_LIBRARY}")
#     message("Brahma library path: ${BRAHMA_LIBRARY}")
# else ()
#     message(FATAL_ERROR "-- [DTIO_INTERCEPTOR] brahma is needed for ${PROJECT_NAME} build")
# endif ()

# Variable for setting the log level (1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE)
set(LOG_LEVEL 1 CACHE STRING "Set the log level")
# Define LOG_LEVEL
add_compile_definitions(LOG_LEVEL=${LOG_LEVEL})

add_library(dtio_common SHARED ${COMMON_SRC})
add_library(${PROJECT_NAME} SHARED ${DTIO_LIB_SRC})
add_library(dtio_posix_interception SHARED ${DTIO_POSIX_INTERCEPTION})
add_library(dtio_stdio_interception SHARED ${DTIO_STDIO_INTERCEPTION})
# add_library(dtio_brahma_interception SHARED ${DTIO_BRAHMA_INTERCEPTION})
# add_executable(dtio_client ${DTIO_CLIENT_SRC})
# add_executable(dtio_server ${DTIO_SERVER_SRC})
add_executable(dtio_task_scheduler ${DTIO_TS_SRC})
add_executable(dtio_worker_manager ${DTIO_WM_SRC})
add_executable(dtio_worker ${DTIO_WS_SRC})
add_executable(dtio_hclmanager ${DTIO_HM_SRC})

function(dtio_set_libs target)
  target_link_libraries(${target} yaml-cpp)
  find_package(FMT REQUIRED)
  target_link_libraries(${target} -lm)
  if(FMT_FOUND)
    target_link_libraries(${target} -lfmt)
  endif()
  if(DEFINED MPI)
    target_link_libraries(${target} ${MPI_LIBRARIES})
  endif()
  if(ROCKS)
    target_link_libraries(${target} -lrocksdb)
  endif()
  if(MEMCACHED)
    target_link_libraries(${target} -lmemcached)
  endif()
  if(DEFINED HCL)
    target_link_libraries(${target} -lhcl)
    target_link_libraries(${target} -lmercury)
    target_link_libraries(${target} -lmargo)
    target_link_libraries(${target} -labt)
    target_link_libraries(${target} -lcpp-logger)
  endif()
  target_link_libraries(${target} -pthread cityhash)
  if(NOT ${target} STREQUAL "dtio_common")
    message(${target})
    add_dependencies(${target} dtio_common)
    target_link_libraries(${target} dtio_common)
  endif()
  target_link_libraries(${target} -lhdf5)
  target_link_libraries(${target} -luring)
endfunction()

function(dtio_set_libs_keyword target)
  target_link_libraries(${target} PRIVATE yaml-cpp)
  find_package(FMT REQUIRED)
  target_link_libraries(${target} PRIVATE -lm)
  if(FMT_FOUND)
    target_link_libraries(${target} PRIVATE -lfmt)
  endif()
  if(DEFINED MPI)
    target_link_libraries(${target} PRIVATE ${MPI_LIBRARIES})
  endif()
  if(ROCKS)
    target_link_libraries(${target} PRIVATE -lrocksdb)
  endif()
  if(MEMCACHED)
    target_link_libraries(${target} PRIVATE -lmemcached)
  endif()
  if(DEFINED HCL)
    target_link_libraries(${target} PRIVATE -lhcl)
    target_link_libraries(${target} PRIVATE -lmercury)
    target_link_libraries(${target} PRIVATE -lmargo)
    target_link_libraries(${target} PRIVATE -labt)
    target_link_libraries(${target} PRIVATE -lcpp-logger)
  endif()
  target_link_libraries(${target} PRIVATE -pthread cityhash)
  if(NOT ${target} STREQUAL "dtio_common")
    message(${target})
    add_dependencies(${target} dtio_common)
    target_link_libraries(${target} PRIVATE dtio_common)
  endif()
  target_link_libraries(${target} PRIVATE -lhdf5)
  target_link_libraries(${target} PRIVATE -luring)
endfunction()

dtio_set_libs(dtio_common)
# dtio_set_libs(dtio_client)
# dtio_set_libs(dtio_server)
dtio_set_libs(dtio_task_scheduler)
dtio_set_libs(dtio_worker_manager)
dtio_set_libs(dtio_worker)
dtio_set_libs(dtio_hclmanager)
dtio_set_libs(${PROJECT_NAME})
dtio_set_libs(dtio_posix_interception)
dtio_set_libs(dtio_stdio_interception)
# dtio_set_libs(dtio_brahma_interception)
add_dependencies(dtio_posix_interception dtio)
add_dependencies(dtio_stdio_interception dtio)
# add_dependencies(dtio_brahma_interception dtio)
target_link_libraries(dtio_posix_interception dtio)
target_link_libraries(dtio_stdio_interception dtio)
# target_link_libraries(dtio_brahma_interception dtio)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES VERSION ${PROJECT_VERSION}
             SOVERSION 1
             PUBLIC_HEADER include/dtio.h)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(dtio_posix_interception PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(dtio_stdio_interception PROPERTIES LINKER_LANGUAGE CXX)
# set_target_properties(dtio_brahma_interception PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PRIVATE .)

target_link_libraries(dtio ${CMAKE_CXX_LINK_FLAGS})
target_link_libraries(dtio_posix_interception ${CMAKE_CXX_LINK_FLAGS})
target_link_libraries(dtio_stdio_interception ${CMAKE_CXX_LINK_FLAGS})
# target_link_libraries(dtio_brahma_interception ${CMAKE_CXX_LINK_FLAGS})
# target_link_libraries(dtio_brahma_interception -lbrahma)
# target_link_libraries(dtio_posix_interception PRIVATE -Wall -fPIC)

install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS dtio_common DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install(TARGETS dtio_client DESTINATION ${CMAKE_INSTALL_BINDIR})
# install(TARGETS dtio_server DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dtio_task_scheduler DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dtio_worker_manager DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dtio_worker DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dtio_hclmanager DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(benchmark)
add_subdirectory(adapter/py-bindings)
#add_subdirectory(adapter)
# add_subdirectory(hermes_shm)

# For VOL
find_package (HDF5 1.14 COMPONENTS C REQUIRED) # 1.14?
link_directories (${HDF5_LIBRARY_DIRS})
include_directories (${HDF5_INCLUDE_DIR})
set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY})

# Set the DTIO VOL connector name
set (TVC_NAME dtio_vol_connector)

# Build the template VOL
add_library (${TVC_NAME} SHARED adapter/vol/H5VLDTIO.cc ${DTIO_LIB_SRC})
dtio_set_libs(${TVC_NAME})
add_dependencies(${TVC_NAME} dtio)
target_link_libraries(${TVC_NAME} dtio)
set_target_properties(${TVC_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${TVC_NAME} ${CMAKE_CXX_LINK_FLAGS})
set_target_properties (${TVC_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties (${TVC_NAME} PROPERTIES SOVERSION 1)
set_target_properties (${TVC_NAME} PROPERTIES PUBLIC_HEADER "${TVC_NAME}.h")
# add_subdirectory (adapter/vol)

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(dtiopy src/lib/dtio.cpp src/common/threadPool.cpp ${DTIO_LIB_SRC})
dtio_set_libs_keyword(dtiopy)
