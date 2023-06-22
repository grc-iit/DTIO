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
  auto msg = serialization_manager ().serialize_task (task_t);
  // TODO: replace with hcl queue
  // natsConnection_PublishString (nc, subject.c_str (), msg.c_str ());
  return 0;
}

task *
HCLQueueImpl::subscribe_task_with_timeout (int &status)
{
  // TODO: replace with HCL
  // natsMsg *msg = nullptr;
  // natsSubscription_NextMsg (&msg, sub, MAX_TASK_TIMER_MS);
  if (msg == nullptr)
    return nullptr;
  task *task
      = serialization_manager ().deserialize_task (natsMsg_GetData (msg));
  status = 0;
  return task;
}

task *
HCLQueueImpl::subscribe_task (int &status)
{
  // TODO: replace with HCL
  // natsMsg *msg = nullptr;
  // natsSubscription_NextMsg (&msg, sub, MAX_TASK_TIMER_MS_MAX);
  if (msg == nullptr)
    return nullptr;
  task *task
      = serialization_manager ().deserialize_task (natsMsg_GetData (msg));
  status = 0;
  return task;
}

int
HCLQueueImpl::get_queue_size ()
{
  // TODO: replace with HCL
  int size_of_queue;
  natsSubscription_GetPending (sub, nullptr, &size_of_queue);
  return size_of_queue;
}

int
HCLQueueImpl::get_queue_count ()
{
  // TODO: replace with HCL
  // int *count_of_queue = new int ();
  // natsSubscription_GetStats (sub, count_of_queue, NULL, NULL, NULL, NULL,
  //                           NULL);
  int queue_count = *count_of_queue;
  delete (count_of_queue);
  return queue_count;
}

int
HCLQueueImpl::get_queue_count_limit ()
{
  return INT_MAX;
}
