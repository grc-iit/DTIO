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

// include files
#include "task_scheduler.h"
#include "dtio/common/logger.h"
#include <algorithm>
#include <dtio/common/data_structures.h>
#include <dtio/common/external_clients/memcached_impl.h>
#include <dtio/dtio_system.h>
#include <iomanip>

std::shared_ptr<task_scheduler> task_scheduler::instance = nullptr;
service task_scheduler::service_i = service (TASK_SCHEDULER);

// Interface
int
task_scheduler::run ()
{
  auto queue = dtio_system::getInstance (service_i)->get_client_queue (
      CLIENT_TASK_SUBJECT);
  auto task_list = std::vector<task *> ();
  Timer t = Timer ();
  int status;
  task *task_i = nullptr;

  DTIO_LOG_TRACE ("[DTIO-TS] Looping");

  // queue->clear(); // Clean out the queue before the task scheduler starts

  while (!kill)
    {
    // NOTE: Change to TRACE cause spawn
    DTIO_LOG_TRACE ("[DTIO-TS] RUN :: START");
      status = -1;
      DTIO_LOG_TRACE ("[DTIO-TS] RUN :: SUBSCRIBING");
      task_i = queue->subscribe_task_with_timeout (status);
      if (status != -1 && task_i != nullptr)
        {
          task_list.push_back (task_i);
        }

      auto time_elapsed = t.pauseTime ();
      DTIO_LOG_TRACE ("[DTIO-TS] RUN :: SUBSCRIBED");
      if (!task_list.empty ()
          && (task_list.size () >= MAX_NUM_TASKS_IN_QUEUE
              || time_elapsed >= MAX_SCHEDULE_TIMER))
        {
          // scheduling_threads.submit(std::bind(schedule_tasks, task_list));
          schedule_tasks (task_list);
          t.resumeTime ();
          task_list.clear ();
        }
    DTIO_LOG_TRACE ("[DTIO-TS] RUN :: EoL");
    }
  DTIO_LOG_ERROR ("[DTIO-TS] DEAD");
  return 0;
}

void
task_scheduler::schedule_tasks (std::vector<task *> &tasks)
{
#ifdef TIMERTS
  Timer t = Timer ();
  t.resumeTime ();
#endif
  auto solver_i = dtio_system::getInstance (service_i)->solver_i;
  solver_input input (tasks, static_cast<int> (tasks.size ()));
  solver_output output = solver_i->solve (input);

  for (auto element : output.worker_task_map)
    {
      auto queue = dtio_system::getInstance (service_i)->get_worker_queue (
          element.first);
      for (auto tsk : element.second)
        {

          switch (tsk->t_type)
            {
            case task_type::WRITE_TASK:
              {
                auto *wt = tsk; //reinterpret_cast<write_task *> (task);
#ifdef DEBUG
                std::cout
                    << "threadID:" << std::this_thread::get_id ()
                    << "\tOperation"
                    << static_cast<std::underlying_type<task_type>::type> (
                           tsk->t_type)
                    << "\tDataspaceID#" << wt->destination.filename
                    << "\tTask#" << tsk->task_id << "\tWorker#"
                    << element.first << "\n";
#endif
                queue->publish_task (wt);
                break;
              }
            case task_type::READ_TASK:
              {
                auto *rt = tsk;
#ifdef DEBUG
                std::cout
                    << "threadID:" << std::this_thread::get_id ()
                    << "\tOperation"
                    << static_cast<std::underlying_type<task_type>::type> (
                           tsk->t_type)
                    << "\tTask#" << tsk->task_id << "\tWorker#"
                    << element.first << "\n";
#endif
                queue->publish_task (rt);
                break;
              }
            }
        }
    }
  for (auto tsk : tasks)
    {
      delete tsk;
    }
  delete input.task_size;
  delete output.solution;
#ifdef TIMERTS
  std::stringstream stream;
  stream << "task_scheduler::schedule_tasks()," << std::fixed
         << std::setprecision (10) << t.pauseTime () << "\n";
  std::cout << stream.str ();
#endif
}
