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
#ifndef DTIO_MAIN_WORKERSERVICE_H
#define DTIO_MAIN_WORKERSERVICE_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include "../task_scheduler/task_scheduler.h"
#include "api/io_client.h"
#include "api/posix_client.h"
#include "api/stdio_client.h"
#include "api/multi_client.h"
#include <dtio/common/external_clients/memcached_impl.h>
#include <dtio/dtio_system.h>
/******************************************************************************
 *Class
 ******************************************************************************/
class worker {
private:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  static std::shared_ptr<worker> instance;
  service service_i;
  int worker_index;
  std::shared_ptr<distributed_queue> queue;
  std::shared_ptr<distributed_hashmap> map;
  std::shared_ptr<io_client> client;
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  worker(service service, int worker_index)
      : service_i(service), kill(false), worker_index(worker_index) {
    if (io_client_type_t == io_client_type::POSIX) {
      client = std::make_shared<posix_client>(posix_client(worker_index));
    }
    else if (io_client_type_t == io_client_type::STDIO) {
      client = std::make_shared<stdio_client>(stdio_client(worker_index));
    }
    else if (io_client_type_t == io_client_type::MULTI) {
      client = std::make_shared<multi_client>(multi_client(worker_index));
    }

    queue =
        dtio_system::getInstance(service_i)->get_worker_queue(worker_index);
    map = dtio_system::getInstance(service_i)->map_server();
  }
  /******************************************************************************
   *Interface
   ******************************************************************************/
  int setup_working_dir();
  int calculate_worker_score(bool before_sleeping);
  int update_capacity();
  int64_t get_total_capacity();
  int64_t get_current_capacity();
  float get_remaining_capacity();
  int update_score(bool before_sleeping);

public:
  int run();
  int kill;
  inline static std::shared_ptr<worker> getInstance(service service,
                                                    int worker_index) {
    return instance == nullptr
               ? instance =
                     std::shared_ptr<worker>(new worker(service, worker_index))
               : instance;
  }
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~worker() {}
};

#endif // DTIO_MAIN_WORKERSERVICE_H
