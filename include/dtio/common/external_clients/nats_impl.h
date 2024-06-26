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
#ifndef DTIO_MAIN_NATSCLIENT_H
#define DTIO_MAIN_NATSCLIENT_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <dtio/common/client_interface/distributed_queue.h>
#include <nats.h>

/******************************************************************************
 *Class
 ******************************************************************************/
class NatsImpl : public distributed_queue {
private:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  natsConnection *nc = nullptr;
  natsSubscription *sub = nullptr;
  std::string subject;

public:
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  NatsImpl(service service, const std::string &url, const std::string &subject,
           std::string queue_group, bool subscribe)
      : distributed_queue(service), subject(subject) {
    natsConnection_ConnectTo(&nc, url.c_str());
    if (subscribe) {
      if (queue_group.empty())
        natsConnection_SubscribeSync(&sub, nc, subject.c_str());
      else
        natsConnection_QueueSubscribeSync(&sub, nc, subject.c_str(),
                                          queue_group.c_str());
    }

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
  virtual ~NatsImpl() {}
};

#endif // DTIO_MAIN_NATSCLIENT_H
