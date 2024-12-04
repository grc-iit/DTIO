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
#ifndef DTIO_MAIN_HCLQUEUEIMPL_H
#define DTIO_MAIN_HCLQUEUEIMPL_H

#include "dtio/common/enumerations.h"
// #include "hcl/queue/queue.h"
#include <hcl.h>
#include <hcl/queue/queue.h>
#include <city.h>
#include <cstring>
#include <dtio/common/client_interface/distributed_queue.h>
#include <dtio/common/config_manager.h>
#include <dtio/common/data_structures.h>
#include <dtio/common/logger.h>
#include <iostream>
#include <string>

class HCLQueueImpl : public distributed_queue
{
private:
  // std::string subject;
  uint16_t tail_subscription;
  uint16_t head_subscription;
  // DONE: comm needs to be a vector(? or arrays or pointers)
  // and barrier over all the barriers
  size_t comm_size = 0;

public:
  hcl::queue<task> *hcl_queue;
  // DONE: make HCLKeyType based on Map
  std::hash<task> task_hash;

  // TODO: config is the same as map but
  // order might be different.
  // need to know what service it is
  // look into barriers and comms at the same time
  // when initing hcl queues put barriers to ensure rpc init
  HCLQueueImpl (service service, const std::string &subject, int my_server,
                int num_servers, bool subscribe, std::shared_ptr<hcl::HCL> &hcl_init)
      : distributed_queue (service)
  {
    try {
    head_subscription = my_server;
    std::string queuename = "";
    // task sched == server, lib != server
    if (service == TASK_SCHEDULER && subject == CLIENT_TASK_SUBJECT)
      {
        HCL_CONF->IS_SERVER = false;
        queuename = "taskscheduler+";
      }
    else if (service == TASK_SCHEDULER && subject != CLIENT_TASK_SUBJECT)
      {
        HCL_CONF->IS_SERVER = false;
        queuename = "worker+";
      }
    else if (service == WORKER)
      {
        HCL_CONF->IS_SERVER = false;
        queuename = "worker+";
      }
    else if (service == LIB)
      {
        HCL_CONF->IS_SERVER = false;
        queuename = "taskscheduler+";
      }
    else if (service == HCLCLIENT && subject == CLIENT_TASK_SUBJECT)
      {
	HCL_CONF->IS_SERVER = true;
	queuename = "taskscheduler+";
      }
    else if (service == HCLCLIENT && subject != CLIENT_TASK_SUBJECT)
      {
	HCL_CONF->IS_SERVER = true;
	queuename = "worker+";
      }
    else
      {
        std::cout << "I'm uncertain why this happens " << service << std::endl;
        queuename = "help+";
        // NOTE: shouldnt we just fail and exit here?
      }
    queuename += (subject + "+");
    
    int worker_index = -1;
    if (subject != CLIENT_TASK_SUBJECT)
      {
        DTIO_LOG_INFO ("[Queue] CLIENT @ " << service << "\t" << subject);
        worker_index = std::stoi (subject);
      }

    // if (subject == CLIENT_TASK_SUBJECT) {
    //   my_server = 0; // 2
    // }
    // else {
    //   my_server = 1 + worker_index; // 3 +
    // }
    HCL_CONF->MY_SERVER = 0; // my_server;
    HCL_CONF->NUM_SERVERS = 1; // num_servers;
    HCL_CONF->SERVER_ON_NODE = HCL_CONF->IS_SERVER || (service == TASK_SCHEDULER && subject == CLIENT_TASK_SUBJECT) || (service == WORKER); // true; // (service != LIB)
    HCL_CONF->SERVER_LIST_PATH = ConfigManager::get_instance ()->HCL_SERVER_LIST_PATH; // "/home/kbateman/server_list";

    // if (service == WORKER || service == TASK_SCHEDULER)
    //   {
    // DONE: need to change how barrier works
    // one for (worker + task scheduler) and (client/lib + task
    // scheduler) in server client, need to ensure rpc is initiated
    // comm changes based on kind of queue

    DTIO_LOG_INFO ("[Queue] Created & Barriering :: Subject "
                   << subject << "\tName: " << queuename);

    // head_subscription = tail_subscription = 0;

    if (service == WORKER && subject != CLIENT_TASK_SUBJECT)
      {
        DTIO_LOG_INFO ("[Queue] @ " << service << "\t"
		       << subject << "\t in WI#"
		       << worker_index);
        MPI_Barrier (
            ConfigManager::get_instance ()->QUEUE_WORKER_COMM[worker_index]);
      }
    if (service == TASK_SCHEDULER && subject == CLIENT_TASK_SUBJECT)
      {
        DTIO_LOG_INFO ("[Queue] @ " << service << "\t"
		       << subject << "\t in WI#"
		       << worker_index);
	MPI_Barrier (
		     ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM); // need a new comm for this
      }


    if (subject == CLIENT_TASK_SUBJECT) {
      if (hcl_init == nullptr) {
	hcl_init = hcl::HCL::GetInstance(true, 9400);
      }
      else {
	hcl_init->ReConfigure(9400);
      }
    }
    else {
      if (hcl_init == nullptr) {
	hcl_init = hcl::HCL::GetInstance(true, 9500 + worker_index * 10);
      }
      else {
	hcl_init->ReConfigure(9500 + worker_index * 10);
      }
    }

    hcl_queue = new hcl::queue<task> (queuename);

    if (service == HCLCLIENT && subject != CLIENT_TASK_SUBJECT) {
      DTIO_LOG_INFO ("[Queue] HCLCLIENT @ " << service << "\t" << subject
		     << "\t in WI#" << worker_index);
      MPI_Barrier (
		   ConfigManager::get_instance ()->QUEUE_WORKER_COMM[worker_index]);
    }
    if (service == HCLCLIENT && subject == CLIENT_TASK_SUBJECT) {
      DTIO_LOG_INFO ("[Queue] HCLCLIENT @ " << service << "\t" << subject);
      MPI_Barrier (
		   ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM);
    }
    DTIO_LOG_INFO ("[Queue] ===>> FIN <<== @ " << service << "\t" << subject
                                               << "\t in WI#" << worker_index);
    } catch (std::logic_error *e) { std::cerr << "Error creating queue on service " << service << ", " << e->what() << std::endl; exit(EXIT_FAILURE); }

  }

  void clear() override;
  int publish_task (task *task_t) override;
  task *subscribe_task_with_timeout (int &status) override;
  task *subscribe_task (int &status) override;
  task *subscribe_task_getter ();
  bool subscribe_task_helper ();
  int get_queue_count () override;
  int get_queue_size () override;
  int get_queue_count_limit () override;
  virtual ~HCLQueueImpl ()
  {
    // delete hcl_queue;
    // hcl->Finalize();
    // hcl.reset();
  }
};

#endif // DTIO_MAIN_HCLQUEUEimpl_H
