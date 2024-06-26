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
#ifndef DTIO_MAIN_SYSTEM_H
#define DTIO_MAIN_SYSTEM_H

#include "dtio/common/logger.h"
#include <climits>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <dtio/common/client_interface/distributed_queue.h>
#include <dtio/common/config_manager.h>
#include <dtio/common/constants.h>
#include <dtio/common/enumerations.h>
#include <dtio/common/external_clients/hcl_map_impl.h>
#include <dtio/common/external_clients/hcl_queue_impl.h>
#include <dtio/common/external_clients/memcached_impl.h>
// #include <dtio/common/external_clients/nats_impl.h>
#include <dtio/common/external_clients/rocksdb_impl.h>
#include <dtio/common/solver/solver.h>
#include <memory>
#include <mpi.h>
#include <string>
#include <vector>

class dtio_system
{
private:
  static std::shared_ptr<dtio_system> instance;
  int application_id;
  service service_i;

  explicit dtio_system (service service)
      : service_i (service), application_id (), client_comm (), client_rank (),
        rank ()
  {
    worker_queues = (std::shared_ptr<distributed_queue> *)calloc (
        ConfigManager::get_instance ()->NUM_WORKERS,
        sizeof (std::shared_ptr<distributed_queue>));

    for (int i = 0; i < ConfigManager::get_instance ()->NUM_WORKERS; i++)
      {
        worker_queues[i] == nullptr;
      }
    init (service_i);
    return;
  }

  void init (service service);
  std::shared_ptr<distributed_hashmap> map_client_, map_server_;

public:
  std::shared_ptr<solver> solver_i;
  std::shared_ptr<distributed_hashmap>
  map_client ()
  {
    return map_client_;
  }
  std::shared_ptr<distributed_hashmap>
  map_server ()
  {
    return map_server_;
  }
  std::shared_ptr<distributed_queue> client_queue;
  std::shared_ptr<distributed_queue> *worker_queues;

  int rank, client_rank;
  MPI_Comm client_comm;

  inline static std::shared_ptr<dtio_system>
  getInstance (service service)
  {
    return instance == nullptr
               ? instance
                 = std::shared_ptr<dtio_system> (new dtio_system (service))
               : instance;
  }
  inline std::shared_ptr<distributed_queue>
  get_client_queue (const std::string &subject)
  {
    DTIO_LOG_INFO ("[DTIO] Getting client queue");
    if (client_queue == nullptr)
      {
        if (service_i == LIB)
          {
            // client_queue = std::make_shared<NatsImpl> (
            //     service_i, ConfigManager::get_instance ()->NATS_URL_CLIENT,
            //     CLIENT_TASK_SUBJECT, std::to_string (service_i), false);
            client_queue = std::make_shared<HCLQueueImpl> (
                service_i, CLIENT_TASK_SUBJECT, 0, 1, false);
          }
        else
          {
            // client_queue = std::make_shared<NatsImpl> (
            //     service_i, ConfigManager::get_instance ()->NATS_URL_CLIENT,
            //     CLIENT_TASK_SUBJECT, std::to_string (service_i), true);
            client_queue = std::make_shared<HCLQueueImpl> (
                service_i, CLIENT_TASK_SUBJECT, 0, 1, true);
          }
      }
      DTIO_LOG_INFO("[DTIO] RETURNING CLIENT QUEUE");
    return client_queue;
  }
  inline std::shared_ptr<distributed_queue>
  get_worker_queue (const int &worker_index)
  {
    DTIO_LOG_INFO ("[DTIO] Getting worker queue");
    if (worker_queues[worker_index] == nullptr)
      {
        DTIO_LOG_INFO ("[DTIO] ===> Init worker queue");
        worker_queues[worker_index] = std::make_shared<HCLQueueImpl> (
            service_i, std::to_string (worker_index), 0, 1, true);
        DTIO_LOG_INFO ("[DTIO] ===> DONE Init worker queue");
      }
    // worker_queues[worker_index] = std::make_shared<NatsImpl> (
    //     service_i, ConfigManager::get_instance ()->NATS_URL_SERVER,
    //     std::to_string (worker_index),
    //     std::to_string (service_i) + "_" + std::to_string (worker_index),
    //     true);
    DTIO_LOG_INFO ("[DTIO] ---> Passing worker queue");
    return worker_queues[worker_index];
  }
  int build_message_key (MPI_Datatype &message);
  int build_message_file (MPI_Datatype &message_file);
  int build_message_chunk (MPI_Datatype &message_chunk);

  virtual ~dtio_system () { free (worker_queues); };
};

#endif // DTIO_MAIN_SYSTEM_H
