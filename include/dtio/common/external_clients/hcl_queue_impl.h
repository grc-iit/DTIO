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
/******************************************************************************
 *include files
 ******************************************************************************/
#include <city.h>
#include <cstring>
#include <dtio/common/client_interface/distributed_queue.h>
#include <hcl.h>

class HCLQueueImpl : public distributed_queue {
private:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  hcl::queue<std::string> *hcl_client;
  std::string subject;

public:
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  HCLQueueImpl(service service)
      : distributed_queue(service) {
    hcl_client = new hcl::queue<std::string>();

    // natsConnection_SubscribeSync(&sub, nc, subject.c_str());
  }
  /******************************************************************************
   *Interface
   ******************************************************************************/
  int publish_task(task *task_t) override;
  task *subscribe_task_with_timeout(int &status) override;
  task *subscribe_task(int &status) override;
  int get_queue_count() override;
  int get_queue_size() override;
  int get_queue_count_limit() override;
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~HCLQueueImpl() {}
};

#endif // DTIO_MAIN_HCLQUEUEimpl_H
