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
#ifndef DTIO_MAIN_HCLQUEUEIMPL_H
#define DTIO_MAIN_HCLQUEUEIMPL_H

#include "hcl/queue/queue.h"
#include <city.h>
#include <cstring>
#include <dtio/common/client_interface/distributed_queue.h>
#include <dtio/common/config_manager.h>
#include <dtio/common/data_structures.h>
#include <hcl.h>

class HCLQueueImpl : public distributed_queue {
private:

  hcl::queue<std::string> *hcl_client;
  std::string subject;

public:
  HCLQueueImpl(service service, std::string queuename, int my_server, int num_servers)
      : distributed_queue(service){
    if (service == LIB) {
      HCL_CONF->IS_SERVER = true;
    }
    else if (service == WORKER || service == TASK_SCHEDULER) {
      HCL_CONF->IS_SERVER = false; // Apparently the task scheduler has access to this too?
    }
    else {
      std::cout << "I'm uncertain why this happens " << service << std::endl;
    }

    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = true; // service == LIB
    HCL_CONF->SERVER_LIST_PATH = ConfigManager::get_instance()->HCL_SERVER_LIST_PATH;

    if (service == WORKER || service == TASK_SCHEDULER) {
      MPI_Barrier(ConfigManager::get_instance()->DATASPACE_COMM); // Wait for clients to initialize maps
    }
    hcl_client = new hcl::queue<HCLKeyType, task>(queuename);
    if (service == LIB) {
      MPI_Barrier(ConfigManager::get_instance()->DATASPACE_COMM); // Tell the workers we've initialized maps
    }
  }
  int publish_task(task *task_t) override;
  task *subscribe_task_with_timeout(int &status) override;
  task *subscribe_task(int &status) override;
  int get_queue_count() override;
  int get_queue_size() override;
  int get_queue_count_limit() override;
  virtual ~HCLQueueImpl() {}
};

#endif // DTIO_MAIN_HCLQUEUEimpl_H
