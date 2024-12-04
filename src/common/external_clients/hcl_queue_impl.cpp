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
#include "dtio/common/constants.h"
#include "dtio/common/data_structures.h"
#include "dtio/common/logger.h"
#include <chrono>
#include <cstdint>
#include <dtio/common/external_clients/hcl_queue_impl.h>
#include <future>
#include <mpi.h>
#include <sys/types.h>
#include <type_traits>


void
HCLQueueImpl::clear()
{
  while (true) {
    auto queue_pop = hcl_queue->Pop (head_subscription);
    auto queue_bool = queue_pop.first;
    if (!queue_bool) {
      return;
    }
  }
}

int
HCLQueueImpl::publish_task (task *task_t)
{
  // DONE: seems unnecessary? replace with HCL
  // head_subscription is t.task_id if equal to -1
  // NOTE:
  // head_subscription and tail_subscription are uint16_t as hcl indexs are
  // unsigned.
  // if something wonky happens, maybe zoidberg?

  // head_subscription
  //     = head_subscription == (uint16_t)-1 ? task_t->task_id : head_subscription;
  // head_subscription = (uint16_t)0;
  // tail_subscription = task_t->task_id;
  // return !hcl_queue->Push (task_t, task_hash (*task_t));
  // return !hcl_queue->Push(*task_t, task_hash.operator()(task_t));
  uint16_t hashValue = static_cast<uint16_t> (task_hash.operator() (task_t));
  return hcl_queue->Push (*task_t, head_subscription); // hashValue
}

task *
HCLQueueImpl::subscribe_task_getter ()
{
  DTIO_LOG_DEBUG ("[QUEUE] SUB GETTER :: INIT");
  // if (head_subscription < tail_subscription)
  //   {
  DTIO_LOG_DEBUG ("[QUEUE] SUB GETTER :: SUBBING");
  auto queue_pop = hcl_queue->Pop (head_subscription);
  DTIO_LOG_DEBUG ("[QUEUE] SUB GETTER :: POP and ++");

  // head_subscription++;
  auto queue_bool = queue_pop.first;
  if (queue_bool) {
    task *hcl_task = new task(queue_pop.second);
    DTIO_LOG_DEBUG ("[QUEUE] SUB GETTER :: RET TASK");
    return hcl_task;
  }
  else {
    return nullptr;
  }
  //   }
  // DTIO_LOG_DEBUG ("[QUEUE] SUB GETTER :: FAIL");
}

bool
HCLQueueImpl::subscribe_task_helper ()
{
  DTIO_LOG_DEBUG ("[QUEUE] SUB HELPER :: INIT\t"<< head_subscription << "\t" << tail_subscription);
  // auto result = hcl_queue->WaitForElement(head_subscription);

  return hcl_queue->WaitForElement(head_subscription);


  // if (head_subscription < tail_subscription)
  //   {
  //     DTIO_LOG_DEBUG ("[QUEUE] SUB HELPER :: PULL!");
  //     return true;
  //   }
  // DTIO_LOG_DEBUG ("[QUEUE] SUB HELPER :: NO PULL");
  // return false;
}

task *
HCLQueueImpl::subscribe_task_with_timeout (int &status)
{
  // DONE: make a while timer < timeout loop and pop the task and wait a bit
  // use defaults from NATS impl
  DTIO_LOG_DEBUG ("[QUEUE] SUB --> with Timeout :: INIT");
  // std::launch::deferred |
  std::future<bool> result
      = std::async (std::launch::async,
                    &HCLQueueImpl::subscribe_task_helper, this);
  DTIO_LOG_DEBUG ("[QUEUE] SUB --> with Timeout :: FUTURE");
  if (result.wait_for (std::chrono::milliseconds (MAX_TASK_TIMER_MS)) == std::future_status::ready)
    {
      DTIO_LOG_DEBUG ("[QUEUE] SUB --> with Timeout :: GET");
      auto response = result.get();
      if (response) {
	status = 1;
	return subscribe_task_getter();
      }
      else {
	return nullptr;
      }
    }
  else
    {
      DTIO_LOG_DEBUG ("[QUEUE] SUB --> with Timeout :: NULL");
      return nullptr;
    }
}

task *
HCLQueueImpl::subscribe_task (int &status)
{
  // DONE: make a while timer < timeout loop and pop the task and wait a bit
  // use defaults from NATS impl
  return subscribe_task_with_timeout(status);
  // std::future<bool> result
  //     = std::async (std::launch::deferred | std::launch::async,
  //                   &HCLQueueImpl::subscribe_task_helper, this);
  // if (result.wait_for (std::chrono::milliseconds (MAX_TASK_TIMER_MS_MAX))
  //     == std::future_status::ready)
  //   {
  //     DTIO_LOG_DEBUG ("[QUEUE] SUB :: GET");
  //     return subscribe_task_getter();
  //   }
  // else
  //   {
  //     return nullptr;
  //   }
}

int
HCLQueueImpl::get_queue_size ()
{
  // uint16_t pos = 0;
  return hcl_queue->Size (head_subscription);
  // return hcl_queue->Size (task_hash (head_subscription));
}

int
HCLQueueImpl::get_queue_count ()
{
  // this is for direct nats compat
  // should ultimately be something like
  // `return hcl_queue->Size (task_hash (head_subscription));`
  // u_int16_t pos = 0;
  try {
  return hcl_queue->Size (head_subscription);
  } catch (...) { std::cerr << "Some exception, also head " << head_subscription << std::endl; exit(EXIT_FAILURE); }
}

int
HCLQueueImpl::get_queue_count_limit ()
{
  return INT_MAX;
}
