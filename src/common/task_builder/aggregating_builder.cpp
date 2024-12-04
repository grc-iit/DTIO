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
/******************************************************************************
 *include files
 ******************************************************************************/
#include <dtio/common/constants.h>
#include <cmath>
#include <dtio/common/metadata_manager/metadata_manager.h>
#include <dtio/common/task_builder/aggregating_builder.h>
#include <vector>

void aggregating_builder::close_aggregation() {
  /* Aggregation closes on seek or read or once filename changes. When
     this happens, we need to package and send the current aggregation
  */

  if (aggregation_tasks.size() == 0) {
    // Just a quick check to avoid this if we don't have to do it
    return;
  }
  
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  auto sub_task = new task(*aggregation_tasks.back());
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto map_server = dtio_system::getInstance (LIB)->map_server ();

  sub_task->t_type = task_type::WRITE_TASK;
  sub_task->task_id = static_cast<int64_t>(
					   std::chrono::duration_cast<std::chrono::microseconds>(
												 std::chrono::system_clock::now().time_since_epoch())
					   .count());
  sub_task->publish = true;
  sub_task->addDataspace = false;

  while (aggregation_tasks.size() > 0) {
    // Smallest offset
    task *agg_tsk = aggregation_tasks.back();
    sub_task->source.offset = std::min(sub_task->source.offset, agg_tsk->source.offset);
    sub_task->destination.offset = std::min(sub_task->destination.offset, agg_tsk->destination.offset);
    aggregation_tasks.pop_back();
  }
  sub_task->destination.size = current_aggregate_size; // Aggregate size
  sub_task->source.size = sub_task->destination.size;

  map_client->put(table::DATASPACE_DB, sub_task->source.filename, aggregate_buffer + sub_task->source.offset, sub_task->destination.size, std::to_string(-1));

  mdm->update_write_task_info (*sub_task, sub_task->destination.filename, sub_task->destination.size);
  // Publish aggregation and wait for a result
  client_queue->publish_task (sub_task);
  while (!map_server->exists (table::WRITE_FINISHED_DB, std::to_string(sub_task->task_id),
			      std::to_string (-1)))
    {
    }
  map_server->remove (table::WRITE_FINISHED_DB, std::to_string(sub_task->task_id),
		      std::to_string (-1));

  current_aggregate_size = 0;
  aggregation_offset = -1;
}

/******************************************************************************
 *Interface
 ******************************************************************************/
std::vector<task *> aggregating_builder::build_write_task(task tsk,
							  const char *data) {
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  auto map_server = dtio_system::getInstance(service_i)->map_server();
  auto tasks = std::vector<task *>();

  file source = tsk.source;

  auto number_of_tasks =
      static_cast<int>(std::ceil((float)(source.size) / MAX_IO_UNIT));
  auto dataspace_id =
      map_server->counter_inc(COUNTER_DB, DATASPACE_ID, std::to_string(-1));

  int64_t data_offset = 0;
  std::size_t remaining_data = source.size;
  std::size_t chunk_index = 0;

  if (aggregation_offset = -1) {
    aggregation_offset = tsk.destination.offset;
  }
  
  auto sub_task = new task(tsk);
  if (aggregation_offset != tsk.destination.offset) {
    DTIO_LOG_DEBUG("Closing aggregate due to seek " << aggregation_offset << " != " << tsk.destination.offset);
    close_aggregation();
  }
  if (aggregation_tasks.size() != 0 && strcmp(tsk.destination.filename, aggregation_tasks.back()->destination.filename) != 0) {
    DTIO_LOG_DEBUG("Closing aggregate due to filename change " << tsk.destination.filename << " != " << aggregation_tasks.back()->destination.filename);
    close_aggregation();
  }

  if (remaining_data < MIN_IO_UNIT) {
    // Task is small, can be aggregated
    sub_task->t_type = task_type::WRITE_TASK;
    sub_task->task_id = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    sub_task->publish = false;
    sub_task->source = file(tsk.source);
    // source filename is used to indicate chunk, because buffer needs to be MAX_IO_UNIT
    sprintf(sub_task->source.filename, "%s%i", tsk.destination.filename, chunk_index);
    sub_task->source.offset = data_offset + tsk.source.offset;
    sub_task->source.size = remaining_data;
    sub_task->destination = file(tsk.destination);
    sub_task->destination.offset = data_offset + tsk.destination.offset;
    sub_task->destination.size = remaining_data;
    
    sub_task->addDataspace = false;

    memcpy(aggregate_buffer + current_aggregate_size, data + sub_task->source.offset, remaining_data);
    if (sub_task->destination.size + current_aggregate_size >= MIN_IO_UNIT) {
      // Aggregation trigger
      sub_task->destination.size = sub_task->destination.size + current_aggregate_size; // Aggregate size
      sub_task->source.size = sub_task->destination.size;
      while (aggregation_tasks.size() > 0) {
	// Smallest offset
	task *agg_tsk = aggregation_tasks.back();
	sub_task->source.offset = std::min(sub_task->source.offset, agg_tsk->source.offset);
	sub_task->destination.offset = std::min(sub_task->destination.offset, agg_tsk->destination.offset);
	aggregation_tasks.pop_back();
      }
      map_client->put(table::DATASPACE_DB, sub_task->source.filename, aggregate_buffer + data_offset + sub_task->source.offset, sub_task->destination.size, std::to_string(-1));
      sub_task->publish = true;
      tasks.emplace_back(sub_task);
      current_aggregate_size = 0;
      aggregation_offset = -1;
    }
    else {
      // Store task for later aggregation
      aggregation_tasks.emplace_back(sub_task);
      current_aggregate_size += sub_task->destination.size;
      aggregation_offset = sub_task->destination.offset + sub_task->destination.size;
    }
  }
  else {
    while (remaining_data > 0) {
      sub_task->t_type = task_type::WRITE_TASK;
      sub_task->task_id = static_cast<int64_t>(
					       std::chrono::duration_cast<std::chrono::microseconds>(
												     std::chrono::system_clock::now().time_since_epoch())
					       .count());
      sub_task->publish = false;
      if (remaining_data >= MAX_IO_UNIT) {
	sub_task->source = file(tsk.source);
	// source filename is used to indicate chunk, because buffer needs to be MAX_IO_UNIT
	sprintf(sub_task->source.filename, "%s%i", tsk.destination.filename, chunk_index);
	chunk_index++;
	sub_task->source.offset = data_offset + tsk.source.offset;
	sub_task->source.size = MAX_IO_UNIT;
	sub_task->destination = file(tsk.destination);
	sub_task->destination.offset = data_offset + tsk.destination.offset;
	sub_task->destination.size = MAX_IO_UNIT;

	map_client->put(table::DATASPACE_DB, sub_task->source.filename, data + data_offset + tsk.source.offset, sub_task->destination.size, std::to_string(-1));
	sub_task->addDataspace = false;

	data_offset += MAX_IO_UNIT;
	remaining_data -= MAX_IO_UNIT;
      }
      else {
	sub_task->source = file(tsk.source);
	// source filename is used to indicate chunk, because buffer needs to be MAX_IO_UNIT
	sprintf(sub_task->source.filename, "%s%i", tsk.destination.filename, chunk_index);
	chunk_index++;
	sub_task->source.offset = data_offset + tsk.source.offset;
	sub_task->source.size = remaining_data;
	sub_task->destination = file(tsk.destination);
	sub_task->destination.offset = data_offset + tsk.destination.offset;
	sub_task->destination.size = remaining_data;

	map_client->put(table::DATASPACE_DB, sub_task->source.filename, data + data_offset + tsk.source.offset, sub_task->destination.size, std::to_string(-1));
	sub_task->addDataspace = false;

	data_offset += remaining_data;
	remaining_data = 0;
      }
    }
  }
  return tasks;
}

std::vector<task> aggregating_builder::build_read_task(task t) {
  auto tasks = std::vector<task>();
  auto mdm = metadata_manager::getInstance(LIB);
  auto map_server = dtio_system::getInstance(service_i)->map_server();
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  auto chunks = mdm->fetch_chunks(t);
  size_t data_pointer = 0;
  int server = static_cast<int>(dtio_system::getInstance(LIB)->rank /
                                PROCS_PER_MEMCACHED);
  // We don't aggregate reads here, because reads expect an immediate response
  close_aggregation();
  for (auto chunk : chunks) {
    auto rt = new task();
    rt->task_id = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    rt->source = chunk.destination;
    rt->destination.offset = 0;
    rt->destination.size = rt->source.size;
    rt->destination.server = server;
    data_pointer += rt->destination.size;
    snprintf(rt->destination.filename, DTIO_FILENAME_MAX, "%d", map_server->counter_inc(COUNTER_DB, DATASPACE_ID, std::to_string(-1)));
    tasks.push_back(*rt);
    // delete (rt);
  }
  return tasks;
}

std::vector<task> aggregating_builder::build_delete_task(task tsk) {
  auto tasks = std::vector<task>();
  auto mdm = metadata_manager::getInstance(LIB);
  auto map_server = dtio_system::getInstance(service_i)->map_server();
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  int server = static_cast<int>(dtio_system::getInstance(LIB)->rank /
                                PROCS_PER_MEMCACHED);
  tasks.push_back(tsk);
  return tasks;
}
