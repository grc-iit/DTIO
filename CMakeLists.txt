cmake_minimum_required(VERSION 3.10)
project(
  dtio
  VERSION 0.0.1
  DESCRIPTION "DTIO: A Data Task I/O system")

set(CMAKE_CXX_STANDARD 17)
include(GNUInstallDirs)
include_directories(include)
include_directories("hermes_shm/include")
include_directories("adapter") # <-- here 

# include_directories("")
include_directories(${INCLUDE_PATH})
link_libraries(${LIBRARY_PATH})
# define ones we have installed
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MEMCACHED false)
set(NATS false)
set(MPI true)
set(HCL true)
#set(ROCKS true) # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERMDM")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERTS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERTB")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERNATS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERDM")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERW")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMER") set(CMAKE_CXX_FLAGS
# "${CMAKE_CXX_FLAGS} -DFTIMER") set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}
# -DCOLLECT")
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
    src/common/client_interface/distributed_hashmap.cpp
    src/common/metadata_manager/metadata_manager.cpp
    src/dtio_system.cpp
    src/common/client_interface/distributed_hashmap.cpp
    src/common/client_interface/distributed_queue.cpp
    src/common/external_clients/serialization_manager.cpp
    src/common/external_clients/rocksdb_impl.cpp
    src/common/external_clients/memcached_impl.cpp
    src/common/external_clients/nats_impl.cpp
    src/common/external_clients/hcl_queue_impl.cpp
    src/common/external_clients/hcl_map_impl.cpp
    src/common/task_builder/task_builder.cpp)
set(DTIO_LIB_SRC src/lib/stdio.cpp src/lib/posix.cpp src/lib/mpi.cpp src/common/threadPool.cpp)
set(DTIO_INTERCEPTION adapter/posix/posix_api.h )
# set(DTIO_CLIENT_SRC src/client/main.cpp src/client/client.cpp
#                       src/common/threadPool.cpp)
# set(DTIO_SERVER_SRC src/system_manager/main.cpp
#                       src/system_manager/system_manager_service.cpp)
set(DTIO_TS_SRC
    src/task_scheduler/main.cpp src/task_scheduler/task_scheduler.cpp
    src/task_scheduler/task_scheduler.h)
set(DTIO_WM_SRC src/worker_manager/main.cpp
                  src/worker_manager/worker_manager_service.cpp)
set(DTIO_WS_SRC
    src/worker/api/posix_client.cpp src/worker/api/stdio_client.cpp src/worker/api/multi_client.cpp src/worker/program_repo/statistics.cpp
    src/worker/main.cpp src/worker/worker.cpp)
set(DTIO_HM_SRC src/common/external_clients/hcl_map_impl/main.cpp
    src/common/external_clients/hcl_map_impl.cpp)
    

if(DEFINED MPI)
  find_package(MPI REQUIRED)
  set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
  set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
  include_directories(${MPI_INCLUDE_PATH})
endif()
if(DEFINED MEMCACHED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lmemcached -DMEMCACHED_P")
endif()
if(DEFINED HCL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lhcl -DHCL_P")
endif()
if(DEFINED ROCKS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DROCKS_P")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# Variable for setting the log level (1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE)
set(LOG_LEVEL 3 CACHE STRING "Set the log level")
# Define LOG_LEVEL
add_compile_definitions(LOG_LEVEL=${LOG_LEVEL})

add_library(dtio_common SHARED ${COMMON_SRC})
add_library(${PROJECT_NAME} SHARED ${DTIO_LIB_SRC})
add_library(dtio_posix_interception SHARED ${DTIO_INTERCEPTION})
# add_executable(dtio_client ${DTIO_CLIENT_SRC})
# add_executable(dtio_server ${DTIO_SERVER_SRC})
add_executable(dtio_task_scheduler ${DTIO_TS_SRC})
add_executable(dtio_worker_manager ${DTIO_WM_SRC})
add_executable(dtio_worker ${DTIO_WS_SRC})
add_executable(dtio_hclmap ${DTIO_HM_SRC})

function(dtio_set_libs target)
  target_link_libraries(${target} yaml-cpp)
  find_package(FMT REQUIRED)
  if(FMT_FOUND)
    target_link_libraries(${target} -lfmt)
  endif()
  if(DEFINED MPI)
    target_link_libraries(${target} ${MPI_LIBRARIES})
  endif()
  if(DEFINED ROCKS)
    target_link_libraries(${target} -lrocksdb)
  endif()
  if(DEFINED MEMCACHED)
    target_link_libraries(${target} -lmemcached)
  endif()
  if(DEFINED NATS)
    target_link_libraries(${target} -lnats)
  endif()
  if(DEFINED HCL)
    target_link_libraries(${target} -lhcl)
  endif()
  target_link_libraries(${target} -pthread cityhash)
  if(NOT ${target} STREQUAL "dtio_common")
    message(${target})
    add_dependencies(${target} dtio_common)
    target_link_libraries(${target} dtio_common)
  endif()
endfunction()

dtio_set_libs(dtio_common)
# dtio_set_libs(dtio_client)
# dtio_set_libs(dtio_server)
dtio_set_libs(dtio_task_scheduler)
dtio_set_libs(dtio_worker_manager)
dtio_set_libs(dtio_worker)
dtio_set_libs(dtio_hclmap)
dtio_set_libs(${PROJECT_NAME})

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES VERSION ${PROJECT_VERSION}
             SOVERSION 1
             PUBLIC_HEADER include/dtio.h)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(dtio_posix_interception PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PRIVATE .)

target_link_libraries(dtio ${CMAKE_CXX_LINK_FLAGS})
target_link_libraries(dtio_posix_interception PRIVATE -Wall -fPIC)

install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS dtio_common DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install(TARGETS dtio_client DESTINATION ${CMAKE_INSTALL_BINDIR})
# install(TARGETS dtio_server DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dtio_task_scheduler DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dtio_worker_manager DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dtio_worker DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dtio_hclmap DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(benchmark)
#add_subdirectory(adapter)
# add_subdirectory(hermes_shm)
