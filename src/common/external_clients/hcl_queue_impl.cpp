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
#include "dtio/common/constants.h"
#include "dtio/common/data_structures.h"
#include <cstdint>
#include <dtio/common/external_clients/hcl_queue_impl.h>
#include <future>
#include <mpi.h>
#include <sys/types.h>

int
HCLQueueImpl::publish_task (task *task_t)
{
  // DONE: seems unnecessary? replace with HCL
  // auto msg = serialization_manager ().serialize_task (task_t);
  // natsConnection_PublishString (nc, subject.c_str (), msg.c_str ());
  // head_subscription is t.task_id if equal to -1
  head_subscription
      = head_subscription == -1 ? task_t->task_id : head_subscription;
  tail_subscription = task_t->task_id;
  // return !hcl_queue->Push (task_t, task_hash (*task_t));
  // return !hcl_queue->Push(*task_t, task_hash.operator()(task_t));
  uint16_t hashValue = static_cast<uint16_t> (task_hash.operator() (task_t));
  return !hcl_queue->Push (*task_t, hashValue);
}

task *
HCLQueueImpl::subscribe_task_helper ()
{
  if (head_subscription < tail_subscription)
    {
      // can be all 0
      auto queue_pop = hcl_queue->Pop (head_subscription);
      head_subscription++;
      auto queue_bool = queue_pop.first;
      task hcl_task = queue_pop.second;
      return &hcl_task;
    }
}

task *
HCLQueueImpl::subscribe_task_with_timeout (int &status)
{
  // DONE: make a while timer < timeout loop and pop the task and wait a bit
  // use defaults from NATS impl
  std::future<task *> result
      = std::async (std::launch::deferred | std::launch::async,
                    &HCLQueueImpl::subscribe_task_helper, this);
  if (result.wait_for (std::chrono::milliseconds (MAX_TASK_TIMER_MS))
      == std::future_status::ready)
    {
      return result.get ();
    }
  else
    {
      return nullptr;
    }
}

task *
HCLQueueImpl::subscribe_task (int &status)
{
  // DONE: make a while timer < timeout loop and pop the task and wait a bit
  // use defaults from NATS impl
  std::future<task *> result
      = std::async (std::launch::deferred | std::launch::async,
                    &HCLQueueImpl::subscribe_task_helper, this);
  if (result.wait_for (std::chrono::milliseconds (MAX_TASK_TIMER_MS_MAX))
      == std::future_status::ready)
    {
      return result.get ();
    }
  else
    {
      return nullptr;
    }
}

int
HCLQueueImpl::get_queue_size ()
{
  uint16_t pos = 0;
  return hcl_queue->Size (pos);
  // return hcl_queue->Size (task_hash (head_subscription));
}

int
HCLQueueImpl::get_queue_count ()
{
  // this is for direct nats compat
  // should ultimately be something like
  // `return hcl_queue->Size (task_hash (head_subscription));`
  u_int16_t pos = 0;
  return hcl_queue->Size (pos);
}

int
HCLQueueImpl::get_queue_count_limit ()
{
  return INT_MAX;
}
