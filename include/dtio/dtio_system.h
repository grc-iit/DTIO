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
 * Created by hariharan on 2/16/18.
 * Updated by akougkas on 6/26/2018
 * Updated by kbateman and nrajesh (nrj5k) since
 ******************************************************************************/
#ifndef DTIO_MAIN_SYSTEM_H
#define DTIO_MAIN_SYSTEM_H

#include <climits>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <dtio/common/client_interface/distributed_queue.h>
#include <dtio/common/config_manager.h>
#include <dtio/common/constants.h>
#include <dtio/common/enumerations.h>
#include <dtio/common/external_clients/hcl_map_impl.h>
#include <dtio/common/external_clients/hcl_queue_impl.h>
#include <dtio/common/external_clients/memcached_impl.h>
#include <dtio/common/external_clients/nats_impl.h>
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

    if (service == LIB)
      {
        MPI_Comm_size (ConfigManager::get_instance ()->PROCESS_COMM,
                       &num_clients);
      }
    else
      {
        int size;
        MPI_Comm_size (MPI_COMM_WORLD, &size);
        num_clients = size - 1 - ConfigManager::get_instance ()->NUM_WORKERS
                      - ConfigManager::get_instance ()->NUM_SCHEDULERS;
      }

    hcl_map_client_ = (std::shared_ptr<distributed_hashmap> *)calloc (
        num_clients, sizeof (std::shared_ptr<distributed_hashmap>));
    init (service_i);
    return;
  }

  void init (service service);
  std::shared_ptr<distributed_hashmap> map_client_, map_server_;
  std::shared_ptr<distributed_hashmap> *hcl_map_client_;

public:
  int num_clients;
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
    if (client_queue == nullptr)
      {
        if (service_i == LIB)
          {
            client_queue = std::make_shared<NatsImpl> (
                service_i, ConfigManager::get_instance ()->NATS_URL_CLIENT,
                CLIENT_TASK_SUBJECT, std::to_string (service_i), false);
          }
        else
          {
            client_queue = std::make_shared<NatsImpl> (
                service_i, ConfigManager::get_instance ()->NATS_URL_CLIENT,
                CLIENT_TASK_SUBJECT, std::to_string (service_i), true);
          }
      }
    return client_queue;
  }
  inline std::shared_ptr<distributed_queue>
  get_worker_queue (const int &worker_index)
  {
    if (worker_queues[worker_index] == nullptr)
      worker_queues[worker_index] = std::make_shared<NatsImpl> (
          service_i, ConfigManager::get_instance ()->NATS_URL_SERVER,
          std::to_string (worker_index),
          std::to_string (service_i) + "_" + std::to_string (worker_index),
          true);
    return worker_queues[worker_index];
  }
  int build_message_key (MPI_Datatype &message);
  int build_message_file (MPI_Datatype &message_file);
  int build_message_chunk (MPI_Datatype &message_chunk);

  virtual ~dtio_system ()
  {
    free (worker_queues);
    free (hcl_map_client_);
  };
};

#endif // DTIO_MAIN_SYSTEM_H
