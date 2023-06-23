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
#include <dtio/common/external_clients/hcl_queue_impl.h>
#include <mpi.h>

int
HCLQueueImpl::publish_task (task *task_t)
{
  // TODO: replace with HCL
  // auto msg = serialization_manager ().serialize_task (task_t);
  // natsConnection_PublishString (nc, subject.c_str (), msg.c_str ());
  task t = *task_t;
  // head_subscription is t.task_id if equal to -1
  head_subscription = head_subscription == -1 ? t.task_id : head_subscription;
  tail_subscription = t.task_id;
  hcl_queue->Push (t, task_hash (t));
  return 0;
}

task *
HCLQueueImpl::subscribe_task_with_timeout (int &status)
{
  // TODO: add timeout to ConfigManager
  // create a timeouted task of 10ms
  return subscribe_task (status);
}

task *
HCLQueueImpl::subscribe_task (int &status)
{
  // FIXME: w. keith Pop vs LocalPop
  if (head_subscription < tail_subscription)
    {
      auto queue_pop = hcl_queue->Pop (task_hash (head_subscription++));
      auto queue_bool = queue_pop.first;
      task hcl_task = queue_pop.second;
      status = 0;
      return &hcl_task;
    }
  else
    {
      return nullptr;
    }
}

int
HCLQueueImpl::get_queue_size ()
{
  // FIXME: w. Keith: LocalSize vs Size?
  return hcl_queue->Size (task_hash (head_subscription));
}

int
HCLQueueImpl::get_queue_count ()
{
  // FIXME: w. Keith: queue count vs queue size?
  return hcl_queue->Size (task_hash (head_subscription));
}

int
HCLQueueImpl::get_queue_count_limit ()
{
  return INT_MAX;
}
