/*
 * Copyright (C) 2023  SCS Lab <scs-help@cs.iit.edu>, Hariharan
 * Devarajan <hdevarajan@hawk.iit.edu>, Anthony Kougkas
 * <akougkas@iit.edu>, Xian-He Sun <sun@iit.edu>
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
/*******************************************************************************
 * Created by hariharan on 5/12/18.
 * Updated by akougkas on 6/26/2018
 ******************************************************************************/
#ifndef DTIO_CONFIGURATION_MANAGER_H
#define DTIO_CONFIGURATION_MANAGER_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <iostream>
#include <dtio/common/path_parser.h>
#include <memory>
#include <string>
#include <thread>
#include <yaml-cpp/yaml.h>
#include <mpi.h>
#include <dtio/common/enumerations.h>

/******************************************************************************
 *Class
 ******************************************************************************/
class ConfigManager {
private:
  YAML::Node config_;
  static std::shared_ptr<ConfigManager> instance;
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  ConfigManager()
      : NATS_URL_CLIENT("nats://localhost:4222/"),
        NATS_URL_SERVER("nats://localhost:4223/"),
        MEMCACHED_URL_CLIENT(
            "--SERVER=localhost:11211 --SERVER=localhost:11212"),
        MEMCACHED_URL_SERVER("--SERVER=localhost:11213"),
        ASSIGNMENT_POLICY("RANDOM"), TS_NUM_WORKER_THREADS(8) {}

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
  // MPI_Comm QUEUE_TASKSCHED_COMM;
  int TS_NUM_WORKER_THREADS;
  std::size_t NUM_WORKERS; // FIXME: make private
  std::size_t NUM_SCHEDULERS; // FIXME: make private

  static std::shared_ptr<ConfigManager> get_instance() {
    return instance == nullptr
               ? instance = std::shared_ptr<ConfigManager>(new ConfigManager())
               : instance;
  }
  void LoadConfig(char *path) {
    config_ = YAML::LoadFile(path);

    HCL_SERVER_LIST_PATH = config_["HCL_SERVER_LIST_PATH"].as<std::string>();
    NATS_URL_CLIENT = config_["NATS_URL_CLIENT"].as<std::string>();
    NATS_URL_SERVER = config_["NATS_URL_SERVER"].as<std::string>();
    MEMCACHED_URL_CLIENT = config_["MEMCACHED_URL_CLIENT"].as<std::string>();
    MEMCACHED_URL_SERVER = config_["MEMCACHED_URL_SERVER"].as<std::string>();
    ASSIGNMENT_POLICY = config_["ASSIGNMENT_POLICY"].as<std::string>();
    WORKER_PATH = scs::path_parser(config_["WORKER_PATH"].as<std::string>());
    PFS_PATH = scs::path_parser(config_["PFS_PATH"].as<std::string>());
    TS_NUM_WORKER_THREADS = config_["TS_NUM_WORKER_THREADS"].as<int>();
    NUM_WORKERS = config_["NUM_WORKERS"].as<int>();
    NUM_SCHEDULERS = config_["NUM_SCHEDULERS"].as<int>();
    QUEUE_WORKER_COMM = (MPI_Comm *)calloc(NUM_WORKERS, sizeof(MPI_Comm));
  }
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~ConfigManager() { free(QUEUE_WORKER_COMM); }
};

#endif // DTIO_CONFIGURATION_MANAGER_H
