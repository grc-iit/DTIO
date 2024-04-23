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
#ifndef DTIO_AGGREGATING_BUILDER_H
#define DTIO_AGGREGATING_BUILDER_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <chrono>
#include <dtio/common/client_interface/distributed_queue.h>
#include <dtio/common/data_structures.h>
#include <dtio/common/enumerations.h>
#include <memory>
#include <vector>
#include <dtio/common/task_builder/task_builder.h>
/******************************************************************************
 *Class
 ******************************************************************************/
class aggregating_builder : public task_builder {
private:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  service service_i;

  size_t min_io_unit;
  size_t current_aggregate_size;

  std::vector<task *> aggregation_tasks;

  bool aggregation_closed;
  char aggregate_buffer[MAX_IO_UNIT];
  /******************************************************************************
   *Constructor
   ******************************************************************************/

public:
  aggregating_builder(service service) : task_builder(service) {
    // Initialize variables used for aggregations
    current_aggregate_size = 0;
    aggregation_tasks = std::vector<task *>();
    aggregation_closed = false; // TODO Aggregation closes on seek or read. The plan is to package the aggregation and send it at next write or before the read, but this will take some work.
  }

  /******************************************************************************
   *Interface
   ******************************************************************************/
  std::vector<task *> build_write_task(task tsk, const char *data) override;
  std::vector<task> build_read_task(task t) override;
  std::vector<task> build_delete_task(task tsk) override;

  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~aggregating_builder() {}
};

#endif // DTIO_AGGREGATING_BUILDER_H
