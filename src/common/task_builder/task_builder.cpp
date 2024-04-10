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
/******************************************************************************
 *include files
 ******************************************************************************/
#include "dtio/common/constants.h"
#include <cmath>
#include <dtio/common/metadata_manager/metadata_manager.h>
#include <dtio/common/task_builder/task_builder.h>
#include <vector>

std::shared_ptr<task_builder> task_builder::instance = nullptr;
/******************************************************************************
 *Interface
 ******************************************************************************/
std::vector<task *> task_builder::build_write_task(task tsk,
						   const char *data) {
  /* Checks data size, if data would fit in buffer then just return
     given task. If data would not fit in buffer then split task into
     multiple subtasks. Is there a cache? If so, we also have to check
     the limits of the cache for putting data in. Could still be
     useful to generate tasks for metadata purposes.

     buffer size 100 task size 350

     Alternatives: 
     - Biggest task first 100 100 100 50
     - Evenly split tasks 87 87 88 88
     - Whatever the fuck it's doing now

     What happens when we exceed memory limits? Are there plans to
     flush/retrieve data.
   */

  size_t data_size = tsk.source.size;
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  auto map_server = dtio_system::getInstance(service_i)->map_server();
  auto tasks = std::vector<task *>();
  // auto sub_task = new write_task(task);
  // tasks.emplace_back(sub_task);
  // return tasks;

  file source = tsk.source;

  auto number_of_tasks =
      static_cast<int>(std::ceil((float)(source.size) / MAX_IO_UNIT));
  auto dataspace_id =
      map_server->counter_inc(COUNTER_DB, DATASPACE_ID, std::to_string(-1));

  std::size_t base_offset =
      (source.offset / MAX_IO_UNIT) * MAX_IO_UNIT + source.offset % MAX_IO_UNIT;
  std::size_t data_offset = 0;
  std::size_t remaining_data = source.size;
  int server = static_cast<int>(dtio_system::getInstance(LIB)->rank /
                                PROCS_PER_MEMCACHED);
  // std::cout<<"rank"<<dtio_system::getInstance(LIB)->rank<<"server:"<<server
  // <<"\n";
  while (remaining_data > 0) {
    std::size_t chunk_index = base_offset / MAX_IO_UNIT;
    auto sub_task = new task(tsk);
    sub_task->t_type = task_type::WRITE_TASK;
    sub_task->task_id = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    // write not aligned to 2 MB offsets
    if (base_offset != chunk_index * MAX_IO_UNIT) {
      DTIO_LOG_TRACE("Write not aligned" << std::endl);
      size_t bucket_offset = base_offset - chunk_index * MAX_IO_UNIT;
      chunk_meta cm;
      bool chunk_avail = map_client->get(table::CHUNK_DB,
					 source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
					 std::to_string(-1), &cm);
      /*************************** chunk in dataspace
       * ******************************/
      if (cm.destination.location == location_type::CACHE) {
	/********* update new data in dataspace **********/
	char chunk_value[MAX_IO_UNIT];
	map_client->get(table::DATASPACE_DB, cm.destination.filename,
			std::to_string(cm.destination.server), chunk_value);

	std::size_t size_to_write = 0;
	if (remaining_data < MAX_IO_UNIT) {
	  size_to_write = remaining_data;
	} else {
	  size_to_write = MAX_IO_UNIT;
	}
	if (remaining_data + bucket_offset > MAX_IO_UNIT) {
	  size_to_write = MAX_IO_UNIT - bucket_offset;
	}
	if (data_size >= bucket_offset + size_to_write) {
	  // TODO Both conditionals seem to do the same thing now, merge them.
	  memcpy(chunk_value + bucket_offset, data + data_offset, size_to_write);
	  // char new_data[bucket_offset];
	  // memcpy(new_data, data + data_offset, bucket_offset);
	  // chunk_value.replace(bucket_offset, size_to_write, new_data);
			      // data.substr(data_offset, bucket_offset));
	} else {
	  // char new_data[size_to_write];
	  // memcpy(new_data, data + data_offset, size_to_write);
	  memcpy(chunk_value + bucket_offset, data + data_offset, size_to_write);
	  // chunk_value = chunk_value.substr(0, bucket_offset) + new_data;
	    // data.substr(data_offset, size_to_write);
	}
	DTIO_LOG_INFO("WRITE Count " << size_to_write << std::endl);
	map_client->put(table::DATASPACE_DB, cm.destination.filename,
			chunk_value, size_to_write, std::to_string(cm.destination.server));
	sub_task->addDataspace = false;
	if (size_to_write == MAX_IO_UNIT) {
	  sub_task->publish = true;
	  sub_task->destination.size = MAX_IO_UNIT;
	  sub_task->destination.offset = 0;
	  sub_task->source.offset = base_offset;
	  sub_task->source.size = size_to_write;
	  strncpy(sub_task->source.filename, source.filename, DTIO_FILENAME_MAX);
	  sub_task->destination.location = location_type::CACHE;
	  strncpy(sub_task->destination.filename, cm.destination.filename, DTIO_FILENAME_MAX);
	  sub_task->destination.server = cm.destination.worker;
	} else {
	  /********* build new task **********/
	  // We've modified this, but still don't really understand what it's doing
	  sub_task->destination.size = size_to_write;
	  sub_task->destination.offset = 0;
	  sub_task->source.offset = base_offset; // chunk_index * MAX_IO_UNIT;
	  sub_task->source.size = size_to_write; // chunk_value.length();
	  strncpy(sub_task->source.filename, source.filename, DTIO_FILENAME_MAX);
	  sub_task->destination.location = location_type::CACHE;
	  strncpy(sub_task->destination.filename, cm.destination.filename, DTIO_FILENAME_MAX);
	  sub_task->destination.server = cm.destination.worker;
	}

      } else {
	DTIO_LOG_TRACE("Chunk in file" << std::endl);
	/****************************** chunk in file
	 * ********************************/
	std::size_t size_to_write = 0;
	if (remaining_data < MAX_IO_UNIT) {
	  size_to_write = remaining_data;
	} else {
	  size_to_write = MAX_IO_UNIT;
	  sub_task->publish = true;
	}
	if (remaining_data + bucket_offset > MAX_IO_UNIT) {
	  size_to_write = MAX_IO_UNIT - bucket_offset;
	}
	sub_task->destination.worker = cm.destination.worker;
	sub_task->destination.size = size_to_write;
	sub_task->destination.offset = bucket_offset;
	sub_task->source.offset = base_offset;
	sub_task->source.size = size_to_write;
	strncpy(sub_task->source.filename, source.filename, DTIO_FILENAME_MAX);
	sub_task->destination.location = location_type::CACHE;
	sub_task->destination.server = server;
	snprintf(sub_task->destination.filename, DTIO_FILENAME_MAX, "%d_%d", dataspace_id, chunk_index);
	sub_task->meta_updated = true;
      }
      std::size_t size_to_write = 0;
      if (remaining_data < MAX_IO_UNIT) {
	size_to_write = remaining_data;
      } else {
	size_to_write = MAX_IO_UNIT;
      }
      if (remaining_data + bucket_offset > MAX_IO_UNIT) {
	size_to_write = MAX_IO_UNIT - bucket_offset;
      }
      base_offset += size_to_write;
      data_offset += size_to_write;
      remaining_data -= size_to_write;
    }
    // write aligned to 2 MB offsets
    else {
      DTIO_LOG_TRACE("Write aligned" << std::endl);
      size_t bucket_offset = 0;
      /******* remaining_data I/O is less than 2 MB *******/
      if (remaining_data < MAX_IO_UNIT) {
        if (!map_client->exists(table::CHUNK_DB,
                                source.filename +
                                    std::to_string(chunk_index * MAX_IO_UNIT),
                                std::to_string(-1))) {
          sub_task->publish = false;
          sub_task->destination.size = remaining_data;
          sub_task->destination.offset = bucket_offset;
          sub_task->source.offset = base_offset;
          sub_task->source.size = sub_task->destination.size;
          strncpy(sub_task->source.filename, source.filename, DTIO_FILENAME_MAX);
          sub_task->destination.location = location_type::CACHE;
          sub_task->destination.server = server;
          snprintf(sub_task->destination.filename, DTIO_FILENAME_MAX, "%d_%d", dataspace_id, chunk_index);
        } else {
	  chunk_meta cm;
	  map_client->get(table::CHUNK_DB,
			  source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
			  std::to_string(-1), &cm);

          /********* chunk in dataspace **********/
          if (cm.destination.location == location_type::CACHE) {
            // update new data in dataspace
            // auto chunk_value =
            //     map_client->get(table::DATASPACE_DB, cm.destination.filename,
            //                     std::to_string(cm.destination.server));
	    char chunk_value[MAX_IO_UNIT];
	    map_client->get(table::DATASPACE_DB, cm.destination.filename,
			    std::to_string(cm.destination.server), chunk_value);
            if (MAX_IO_UNIT >= bucket_offset + remaining_data) {
	      // FIXME you can't get the size of a buffer from strlen
	      // char new_data[remaining_data];
	      // memcpy(new_data, data + data_offset, remaining_data);
	      memcpy(chunk_value, data + data_offset + bucket_offset, remaining_data);
              // chunk_value.replace(bucket_offset, remaining_data, new_data);
                                  // data.substr(data_offset, remaining_data));
            } else {
	      // char new_data[remaining_data];
	      // memcpy(new_data, data + data_offset, remaining_data);
	      memcpy(chunk_value + bucket_offset, data + data_offset, remaining_data);
              // chunk_value = chunk_value.substr(0, bucket_offset - 1) + new_data;
                            // data.substr(data_offset, remaining_data);
            }
            map_client->put(table::DATASPACE_DB, cm.destination.filename,
                            chunk_value, remaining_data, std::to_string(cm.destination.server));
            // build new task
            sub_task->addDataspace = false;
            sub_task->publish = false;
            sub_task->destination.size = remaining_data; // cm
                                                         //  .destination
                                                         //  .size;
            sub_task->destination.offset = bucket_offset;
            sub_task->source.offset = base_offset;
            sub_task->source.size = remaining_data;
            strncpy(sub_task->source.filename, source.filename, DTIO_FILENAME_MAX);
            sub_task->destination.location = location_type::CACHE;
            sub_task->destination.server = cm.destination.server;
            strncpy(sub_task->destination.filename, cm.destination.filename, DTIO_FILENAME_MAX);
          }
          /************ chunk in file *************/
          else {
	    DTIO_LOG_TRACE("Chunk in file" << std::endl);
            sub_task->publish = false;
            sub_task->destination.worker = cm.destination.worker;
            sub_task->destination.size = remaining_data;
            sub_task->destination.offset = bucket_offset;
	    DTIO_LOG_TRACE("Building task f offset " << base_offset << std::endl);
            sub_task->source.offset = base_offset;
            sub_task->source.size = remaining_data;
            strncpy(sub_task->source.filename, source.filename, DTIO_FILENAME_MAX);
            sub_task->destination.location = location_type::CACHE;
            sub_task->destination.server = server;
            snprintf(sub_task->destination.filename, DTIO_FILENAME_MAX, "%d_%d", dataspace_id, chunk_index);
            sub_task->meta_updated = true;
          }
        }

        base_offset += remaining_data;
        data_offset += remaining_data;
        remaining_data = 0;
      }
      /************** remaining_data is >= 2 MB ***********/
      else {
        sub_task->publish = true;
        sub_task->destination.size = MAX_IO_UNIT;
        sub_task->destination.offset = bucket_offset;
        sub_task->source.offset = base_offset;
        sub_task->source.size = MAX_IO_UNIT;
        strncpy(sub_task->source.filename, source.filename, DTIO_FILENAME_MAX);
        sub_task->destination.location = location_type::CACHE;
        sub_task->destination.server = server;
        snprintf(sub_task->destination.filename, DTIO_FILENAME_MAX, "%d_%d", dataspace_id, chunk_index);
        base_offset += MAX_IO_UNIT;
        data_offset += MAX_IO_UNIT;
        remaining_data -= MAX_IO_UNIT;
      }
    }
    tasks.emplace_back(sub_task);
  }
  return tasks;
}

std::vector<task> task_builder::build_read_task(task t) {
  auto tasks = std::vector<task>();
  auto mdm = metadata_manager::getInstance(LIB);
  auto map_server = dtio_system::getInstance(service_i)->map_server();
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  auto chunks = mdm->fetch_chunks(t);
  size_t data_pointer = 0;
  int server = static_cast<int>(dtio_system::getInstance(LIB)->rank /
                                PROCS_PER_MEMCACHED);
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

std::vector<task> task_builder::build_delete_task(task tsk) {
  auto tasks = std::vector<task>();
  auto mdm = metadata_manager::getInstance(LIB);
  auto map_server = dtio_system::getInstance(service_i)->map_server();
  auto map_client = dtio_system::getInstance(service_i)->map_client();
  int server = static_cast<int>(dtio_system::getInstance(LIB)->rank /
                                PROCS_PER_MEMCACHED);
  tasks.push_back(tsk);
  return tasks;
}
