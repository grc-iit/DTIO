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

#include "dtio/common/enumerations.h"
#include "hcl/queue/queue.h"
#include <city.h>
#include <cstring>
#include <dtio/common/client_interface/distributed_queue.h>
#include <dtio/common/config_manager.h>
#include <dtio/common/data_structures.h>

class HCLQueueImpl : public distributed_queue
{
private:
  hcl::queue<task> *hcl_queue;
  std::string subject;
  uint16_t tail_subscription;
  uint16_t head_subscription;
  // DONE: comm needs to be a vector(? or arrays or pointers)
  // and barrier over all the barriers
  MPI_Comm comm[4];
  size_t comm_size = 0;

public:
  // DONE: make HCLKeyType based on Map
  std::hash<task> task_hash;

  // TODO: config is the same as map but
  // order might be different.
  // need to know what service it is
  // look into barriers and comms at the same time
  // when initing hcl queues put barriers to ensure rpc init
  HCLQueueImpl (service service, const std::string &subject, std::string queuename,
                int my_server, int num_servers, bool subscribe)
      : distributed_queue (service)
  {
    queuename = "";
    // task sched == server, lib != server
    if (service == TASK_SCHEDULER)
      {
        HCL_CONF->IS_SERVER = true;
        queuename = "taskscheduler+" + subject;
        comm[comm_size++]
            = ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM;
      }
    else if (service == WORKER)
      {
        HCL_CONF->IS_SERVER = true;
        queuename = "taskscheduler+" + subject;
        comm[comm_size++] = ConfigManager::get_instance ()->QUEUE_WORKER_COMM;
      }
    else if (service == LIB)
      {
        // Apparently the task scheduler has access to this too?
        HCL_CONF->IS_SERVER = false;
        queuename = "taskscheduler+" + subject;
        comm[comm_size++] = ConfigManager::get_instance ()->QUEUE_CLIENT_COMM;
      }
    else
      {
        std::cout << "I'm uncertain why this happens " << service << std::endl;
        queuename = "help+" + subject;
        comm[comm_size++] = ConfigManager::get_instance ()->QUEUE_CLIENT_COMM;
        // NOTE: shouldnt we just fail and exit here?
      }

    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = true; // service == LIB
    HCL_CONF->SERVER_LIST_PATH
        = ConfigManager::get_instance ()->HCL_SERVER_LIST_PATH;

    if (service == WORKER || service == TASK_SCHEDULER)
      {
        // DONE: need to change how barrier works
        // one for (worker + task scheduler) and (client/lib + task scheduler)
        // in server client, need to ensure rpc is initiated
        // comm changes based on kind of queue

        for (size_t i = 0; i < comm_size; i++)
          {
            MPI_Barrier (comm[i]);
          }
        // Wait for clients to initialize maps
        MPI_Barrier (ConfigManager::get_instance ()->QUEUE_CLIENT_COMM);
      }

    if (service == LIB)
      {
        MPI_Barrier (ConfigManager::get_instance ()
                         ->QUEUE_CLIENT_COMM); // Tell the workers we've
                                               // initialized queues
      }
    head_subscription = tail_subscription = -1;
    hcl_queue = new hcl::queue<task> (queuename);
  }
  int publish_task (task *task_t) override;
  task *subscribe_task_with_timeout (int &status) override;
  task *subscribe_task (int &status) override;
  task *subscribe_task_helper ();
  int get_queue_count () override;
  int get_queue_size () override;
  int get_queue_count_limit () override;
  virtual ~HCLQueueImpl () {}
};

#endif // DTIO_MAIN_HCLQUEUEimpl_H
