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
#include <dtio/common/task_builder/default_builder.h>
#include <vector>

/******************************************************************************
 *Interface
 ******************************************************************************/
std::vector<task *> default_builder::build_write_task(task tsk,
						   const char *data) {
  /* Checks data size, if data would fit in buffer then just return
     given task. If data would not fit in buffer then split task into
     multiple subtasks. Is there a cache? If so, we also have to check
     the limits of the cache for putting data in. Could still be
     useful to generate tasks for metadata purposes.

     buffer size 100 task size 350

     Alternatives: 
     - Biggest task first 100 100 100 50 (current)
     - Evenly split tasks 87 87 88 88

     What happens when we exceed memory limits? Are there plans to
     flush/retrieve data.
   */

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

  // std::size_t chunk_index = dataspace_id;
  while (remaining_data > 0) {
    auto sub_task = new task(tsk);
    sub_task->t_type = task_type::WRITE_TASK;
    sub_task->task_id = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    if (remaining_data >= MAX_IO_UNIT) {
      sub_task->source = file(tsk.source);
      // source filename is used to indicate chunk, because buffer needs to be MAX_IO_UNIT
      sprintf(sub_task->source.filename, "%s%i", tsk.destination.filename, dataspace_id % CHUNK_LIMIT);
      dataspace_id = map_server->counter_inc(COUNTER_DB, DATASPACE_ID, std::to_string(-1));
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
      sprintf(sub_task->source.filename, "%s%i", tsk.destination.filename, dataspace_id % CHUNK_LIMIT);
      dataspace_id = map_server->counter_inc(COUNTER_DB, DATASPACE_ID, std::to_string(-1));
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

    tasks.emplace_back(sub_task);
  }
  return tasks;
}

std::vector<task> default_builder::build_read_task(task t) {
  auto tasks = std::vector<task>();
  auto mdm = metadata_manager::getInstance(LIB);
  auto map_server = dtio_system::getInstance(service_i)->map_server();
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  // auto chunks = mdm->fetch_chunks(t);
  // size_t data_pointer = 0;
  int server = static_cast<int>(dtio_system::getInstance(LIB)->rank /
                                PROCS_PER_MEMCACHED);

    // FIXME DataTasks are supposed to get split up into smaller DataTasks for parsing. This logic was removed when chunking was reorganized, but it needs to be reintroduced eventually. For now, let's simply split reads manually in the program
  auto dataspace_id =
    map_server->counter_inc(COUNTER_DB, DATASPACE_ID, std::to_string(-1));
  int64_t data_offset = 0;
  std::size_t remaining_data = t.source.size;
  if (!t.source.location == BUFFERS && t.source.size > MAX_IO_UNIT) {
    // Sometimes we have to split read tasks, but try to avoid it when we do caching
    while (remaining_data > 0) {
      auto sub_task = new task(t);
      sub_task->t_type = task_type::READ_TASK;
      sub_task->task_id = static_cast<int64_t>(
					       std::chrono::duration_cast<std::chrono::microseconds>(
												     std::chrono::system_clock::now().time_since_epoch())
					       .count());
      if (remaining_data >= MAX_IO_UNIT) {
	sub_task->destination = file(t.destination);
	// dest filename is used to indicate chunk, because buffer needs to be MAX_IO_UNIT
	sprintf(sub_task->destination.filename, "%s%i", t.source.filename, dataspace_id % CHUNK_LIMIT);
	dataspace_id = map_server->counter_inc(COUNTER_DB, DATASPACE_ID, std::to_string(-1));
	sub_task->destination.offset = data_offset + t.destination.offset;
	sub_task->destination.size = MAX_IO_UNIT;

	sub_task->source = file(t.source);
	sub_task->source.offset = data_offset + t.source.offset;
	sub_task->source.size = MAX_IO_UNIT;

	sub_task->addDataspace = false;

	data_offset += MAX_IO_UNIT;
	remaining_data -= MAX_IO_UNIT;
      }
      else {
	sub_task->source = file(t.destination);
	// dest filename is used to indicate chunk, because buffer needs to be MAX_IO_UNIT
	sprintf(sub_task->destination.filename, "%s%i", t.destination.filename, dataspace_id % CHUNK_LIMIT);
	dataspace_id = map_server->counter_inc(COUNTER_DB, DATASPACE_ID, std::to_string(-1));
	sub_task->destination.offset = data_offset + t.destination.offset;
	sub_task->destination.size = remaining_data;

	sub_task->source = file(t.source);
	sub_task->source.offset = data_offset + t.source.offset;
	sub_task->source.size = remaining_data;

	// map_client->put(table::DATASPACE_DB, sub_task->source.filename, data + data_offset + tsk.source.offset, sub_task->destination.size, std::to_string(-1));
	sub_task->addDataspace = false;

	data_offset += remaining_data;
	remaining_data = 0;
      }

      tasks.push_back(*sub_task);
    }
    
  }
  else {
    t.publish = true;
    // t.source.location = BUFFERS;
    t.destination.size = t.source.size;
    tasks.push_back(t);
  }
  return tasks;
}

std::vector<task> default_builder::build_delete_task(task tsk) {
  auto tasks = std::vector<task>();
  auto mdm = metadata_manager::getInstance(LIB);
  auto map_server = dtio_system::getInstance(service_i)->map_server();
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  int server = static_cast<int>(dtio_system::getInstance(LIB)->rank /
                                PROCS_PER_MEMCACHED);
  tasks.push_back(tsk);
  return tasks;
}
