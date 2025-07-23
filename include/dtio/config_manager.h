/*
 * Copyright (C) 2024 Gnosis Research Center <grc@iit.edu>,
 * Keith Bateman <kbateman@hawk.iit.edu>, Neeraj Rajesh
 * <nrajesh@hawk.iit.edu> Hariharan Devarajan
 * <hdevarajan@hawk.iit.edu>, Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of DTIO
 *
 * DTIO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef DTIO_CONFIGURATION_MANAGER_H
#define DTIO_CONFIGURATION_MANAGER_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <dtio/enumerations.h>
#include <dtio/logger.h>
#include <hermes_shm/util/config_parse.h>
#include <iostream>
#include <memory>
#include <mpi.h>
#include <string>
#include <thread>
#include <wordexp.h>
#include <yaml-cpp/yaml.h>

/******************************************************************************
 *Class
 ******************************************************************************/
class ConfigManager
{
private:
  YAML::Node config_;
  static std::shared_ptr<ConfigManager> instance;
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  ConfigManager ()
      : NATS_URL_CLIENT ("nats://localhost:4222/"),
        NATS_URL_SERVER ("nats://localhost:4223/"),
        MEMCACHED_URL_CLIENT (
            "--SERVER=localhost:11211 --SERVER=localhost:11212"),
        MEMCACHED_URL_SERVER ("--SERVER=localhost:11213"),
        ASSIGNMENT_POLICY ("RANDOM"), TS_NUM_WORKER_THREADS (8)
  {
  }

public:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  std::string NATS_URL_CLIENT;
  std::string NATS_URL_SERVER;
  std::string MEMCACHED_URL_CLIENT;
  std::string MEMCACHED_URL_SERVER;
  std::string ASSIGNMENT_POLICY;
  std::string WORKER_PATH;
  std::string PFS_PATH;
  std::string HCL_SERVER_LIST_PATH;
  int test;
  MPI_Comm DATASPACE_COMM;
  MPI_Comm METADATA_COMM;
  MPI_Comm PROCESS_COMM;
  // MPI_Comm QUEUE_CLIENT_COMM;
  MPI_Comm *QUEUE_WORKER_COMM;
  MPI_Comm QUEUE_TASKSCHED_COMM;
  int TS_NUM_WORKER_THREADS;
  std::size_t NUM_WORKERS;    // FIXME: make private
  std::size_t NUM_SCHEDULERS; // FIXME: make private
  bool CHECKFS;     // Check filesystem for file existence, slower but allows
                    // convenient data import
  bool NEVER_TRACE; // Never perform stacktracing, if the path is not
                    // a dtio:// path then send directly to real api
  bool ASYNC;       // Replace synchronous calls with asynchronous equivalents
  bool USE_URING;   // Use io_uring client or posix calls
  bool USE_CACHE;
  size_t WORKER_STAGING_SIZE; // Amount of local space to reserve in a worker
                              // for file staging

  static std::shared_ptr<ConfigManager>
  get_instance ()
  {
    return instance == nullptr
               ? instance
                 = std::shared_ptr<ConfigManager> (new ConfigManager ())
               : instance;
  }
  void
  LoadConfig ()
  {
    // This version loads the config with an environment variable path
    DTIO_LOG_TRACE ("[ConfigManager] Init wordexp");
    wordexp_t path_traverse;
    DTIO_LOG_TRACE ("[ConfigManager] Parsing $DTIO_CONF_PATH");
    wordexp ("${DTIO_CONF_PATH}", &path_traverse, 0);
    if (path_traverse.we_wordc != 1)
      {
        std::cerr
            << "Passed HCL server list path is more than one word on expansion"
            << std::endl;
      }
    DTIO_LOG_TRACE ("[ConfigManager] Parsed $DTIO_CONF_PATH");
    char **words = path_traverse.we_wordv;
    LoadConfig (words[0]);
  }
  void
  LoadConfig (char *path)
  {
    config_ = YAML::LoadFile (path);

    HCL_SERVER_LIST_PATH = "";
    wordexp_t path_traverse;
    DTIO_LOG_TRACE ("[ConfigManager] Parsing HCL_SERVER_LIST_PATH");
    wordexp (config_["HCL_SERVER_LIST_PATH"].as<std::string> ().c_str (),
             &path_traverse, 0);
    if (path_traverse.we_wordc != 1)
      {
        std::cerr
            << "Passed HCL server list path is more than one word on expansion"
            << std::endl;
      }
    DTIO_LOG_TRACE ("[ConfigManager] Parsed HCL_SERVER_LIST_PATH");
    char **words = path_traverse.we_wordv;
    wordfree (&path_traverse);
    ASSIGNMENT_POLICY = config_["ASSIGNMENT_POLICY"].as<std::string> ();
    WORKER_PATH = hshm::ConfigParse::ExpandPath (
        config_["WORKER_PATH"].as<std::string> ());
    PFS_PATH = hshm::ConfigParse::ExpandPath (
        config_["PFS_PATH"].as<std::string> ());
    TS_NUM_WORKER_THREADS = config_["TS_NUM_WORKER_THREADS"].as<int> ();
    NUM_WORKERS = config_["NUM_WORKERS"].as<int> ();
    NUM_SCHEDULERS = config_["NUM_SCHEDULERS"].as<int> ();
    CHECKFS = config_["CHECK_FS"].as<bool> ();
    NEVER_TRACE = config_["NEVER_TRACE"].as<bool> ();
    ASYNC = config_["ASYNC_MODE"].as<bool> ();
    USE_URING = config_["USE_URING"].as<bool> ();
    USE_CACHE = config_["USE_CACHE"].as<bool> ();
    WORKER_STAGING_SIZE = config_["WORKER_STAGING_SIZE"].as<size_t> ();
    QUEUE_WORKER_COMM = (MPI_Comm *)calloc (NUM_WORKERS, sizeof (MPI_Comm));
    DTIO_LOG_TRACE ("[ConfigManager] Parsed Config: " << CHECKFS << " "
                                                      << QUEUE_WORKER_COMM);
  }
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~ConfigManager () { free (QUEUE_WORKER_COMM); }
};

#endif // DTIO_CONFIGURATION_MANAGER_H
