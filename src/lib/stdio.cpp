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
#include "dtio/common/logger.h"
#include <iomanip>
#include <dtio/common/return_codes.h>
#include <dtio/common/task_builder/task_builder.h>
#include <hcl/common/debug.h>
#include <dtio/drivers/stdio.h>
#include <zconf.h>

/******************************************************************************
 *Interface
 ******************************************************************************/
FILE *dtio::stdio::fopen(const char *filename, const char *mode) {
  auto mdm = metadata_manager::getInstance(LIB);
  FILE *fh = nullptr;
  bool file_exists_in_fs = false;
  bool publish_task = false;
  if (!mdm->is_created(filename)) {
    if (strcmp(mode, "a") == 0 && strcmp(mode, "w") == 0) {
      int existing_size = 0;
      if (ConfigManager::get_instance ()->CHECKFS)
	{
	  struct stat st;
	  if (stat ((strncmp(filename, "dtio://", 7) == 0) ? (filename + 7) : filename, &st) == 0)
	    {
	      file_exists_in_fs = true;
	      existing_size = st.st_size;
	    }
	}
      if (!file_exists_in_fs)
	{
	  return nullptr;
	}
      else
	{
	  if (mdm->update_on_open (filename, mode, fh, existing_size) != SUCCESS)
	    {
	      throw std::runtime_error ("dtio::fopen() update failed!");
	    }
	  publish_task = true;
	}
    } else {
      if (mdm->create(filename, mode, fh) != SUCCESS) {
        throw std::runtime_error("dtio::fopen() create failed!");
      }
      publish_task = true;
    }
  } else {
    if (!mdm->is_opened(filename)) {
      if (mdm->update_on_open(filename, mode, fh) != SUCCESS) {
        throw std::runtime_error("dtio::fopen() update failed!");
      }
      publish_task = true;
    } else {
      return nullptr;
    }
  }
  if (publish_task && ConfigManager::get_instance()->WORKER_STAGING_SIZE != 0) {
    // Publish staging task
    auto f = file (std::string (filename), 0, 0);
    auto s_task = task (task_type::STAGING_TASK, f);
    auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
    client_queue->publish_task (&s_task);
  }
  return fh;
}

int dtio::stdio::fclose(FILE *stream) {
  auto mdm = metadata_manager::getInstance(LIB);
  if (!mdm->is_opened(stream))
    return LIB__FCLOSE_FAILED;
  if (mdm->update_on_close(stream) != SUCCESS)
    return LIB__FCLOSE_FAILED;
  return SUCCESS;
}

int dtio::stdio::fseek(FILE *stream, long int offset, int origin) {
  auto mdm = metadata_manager::getInstance(LIB);
  if (!mdm->is_opened(stream)) {
    return EBADF;
  }
 auto filename = mdm->get_filename(stream);
 file_stat st = mdm->get_stat(filename);
 // FIXME may require bigger metadata adjustments, we don't seem to track STDIO mode yet
 //  if (mdm->get_mode(filename) == "a" || mdm->get_mode(filename) == "a+")
 //    return 0;
  auto size = st.file_size;
  auto fp = st.file_pointer;
  switch (origin) {
  case SEEK_SET:
    // if (offset > size)
    //   return -1;
    break;
  case SEEK_CUR:
    // fp + offset > size ||
    if (fp + offset < 0)
      return -1;
    break;
  case SEEK_END:
    // if (offset > 0)
    //   return -1;
    break;
  default:
    // fprintf(stderr, "Seek origin fault!\n");
    return -1;
  }
  if (!mdm->is_opened(stream))
    return -1;
  return mdm->update_on_seek(filename, static_cast<size_t>(offset),
                             static_cast<size_t>(origin));
}

size_t dtio::stdio::fread(void *ptr, size_t size, size_t count, FILE *stream, bool special_type) {
  auto mdm = metadata_manager::getInstance(LIB);
  auto client_queue =
      dtio_system::getInstance(LIB)->get_client_queue(CLIENT_TASK_SUBJECT);
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance(LIB);
  auto filename = mdm->get_filename(stream);
  file_stat st = mdm->get_stat (filename);
  auto offset = st.file_pointer;
  bool check_fs = false;
  task *task_i;
  task_i = nullptr;

  if (!mdm->is_opened(filename)) {
    if (!ConfigManager::get_instance ()->CHECKFS)
      {
	return 0;
      }
    if (!mdm->is_created (filename))
      {
	check_fs = true;
      }
  }
  auto r_task = task(task_type::READ_TASK, file(filename, offset, size * count), file());
  r_task.check_fs = check_fs;
  r_task.special_type = special_type;
  if (ConfigManager::get_instance()->USE_CACHE) {
    r_task.source.location = BUFFERS;
  }
  else {
    r_task.source.location = PFS;
  }

  r_task.iface = io_client_type::STDIO;
#ifdef TIMERTB
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  auto tasks = task_m->build_read_task(r_task);
  DTIO_LOG_TRACE("Task len " << tasks.size() << std::endl);
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_read_task()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  DTIO_LOG_TRACE(stream1.str ());
#endif
  long long int ptr_pos = 0;
  size_t size_read = 0;
  task *ttp;
  for (auto t : tasks) {
    std::string data;
#if(STACK_ALLOCATION)
    {
      char char_data[MAX_IO_UNIT];
      switch (t.source.location) {
      case PFS: {
	DTIO_LOG_DEBUG ("in pfs");
	if (ConfigManager::get_instance ()->CHECKFS)
	  {
	    hcl::Timer timer = hcl::Timer ();
	    client_queue->publish_task (&t);
	    
	    while (!data_m->exists (DATASPACE_DB, t.source.filename,
				    std::to_string (t.destination.server)))
	      {
	      }
	    
	    data_m->get (DATASPACE_DB, t.source.filename,
			 std::to_string (t.destination.server), char_data);
	    
	    strncpy ((char *)ptr + ptr_pos,
		     char_data + t.source.offset, t.destination.size);
	    auto print_test = std::string ((char *)ptr);

	    data_m->remove (DATASPACE_DB, t.source.filename,
			    std::to_string (t.destination.server));
	    
	    size_read += t.source.size;
	  }
	else
	  {
	    DTIO_LOG_DEBUG ("in pfs no checkfs");
	  }
      }
	break;
      case BUFFERS: {
	hcl::Timer timer = hcl::Timer();
	client_queue->publish_task(&t);

	while (!data_m->exists(DATASPACE_DB, t.source.filename,
			       std::to_string(t.destination.server))) {
	  // std::cerr<<"looping\n";
	}
	data_m->get(DATASPACE_DB, t.source.filename,
		    std::to_string(t.destination.server), char_data);
	
	strncpy((char *)ptr + ptr_pos, char_data + t.source.offset,
	       t.destination.size);

	data_m->remove(DATASPACE_DB, t.source.filename,
		       std::to_string(t.destination.server));

	size_read += t.destination.size;
	break;
      }
      case CACHE: {
	DTIO_LOG_TRACE("Cache" << std::endl);
	data_m->get(DATASPACE_DB, t.source.filename,
		    std::to_string(t.destination.server), char_data);
	DTIO_LOG_TRACE("Doing memcpy " << ptr_pos << " " << t.source.offset << " " << t.destination.size << " " << t.source.size << std::endl);
	memcpy((char *)ptr + ptr_pos, char_data + (t.source.offset % t.source.size),
	       t.destination.size);
	size_read += t.destination.size;
	break;
      }
      }
    }
#else
    {
      char *char_data = (char *)malloc(MAX_IO_UNIT);
      switch (t.source.location)
	{
	case PFS:
	  {
	    DTIO_LOG_DEBUG ("in pfs");
	    if (ConfigManager::get_instance ()->CHECKFS)
	      {
		// hcl::Timer timer = hcl::Timer ();
		client_queue->publish_task (&t);

		while (!data_m->exists (DATASPACE_DB, t.source.filename,
					std::to_string (t.destination.server)))
		  {
		  }

		data_m->get (DATASPACE_DB, t.source.filename,
			     std::to_string (t.destination.server), char_data);

		memcpy ((char *)ptr + ptr_pos,
			 char_data, t.source.size);
		// auto print_test = std::string ((char *)ptr);
		// std::cout << "Contents: " << print_test << std::endl;

		data_m->remove (DATASPACE_DB, t.source.filename,
				std::to_string (t.destination.server));

		size_read += t.source.size;
	      }
	    else
	      {
		DTIO_LOG_DEBUG ("in pfs no checkfs");
	      }
	    break;
	  }
	case BUFFERS:
	  {
	      // hcl::Timer timer = hcl::Timer ();
	      // client_queue->publish_task (&t);
	      // while (!data_m->exists (DATASPACE_DB, t.source.filename,
	      // 			      std::to_string (t.destination.server)))
	      // 	{
	      // 	  // std::cerr<<"looping\n";
	      // 	}

	      auto map_client = dtio_system::getInstance(LIB)->map_client();
	      auto map_fm_client = dtio_system::getInstance(LIB)->map_client("metadata+filemeta");

	      // Query the metadata maps to get the datatasks associated with a file
	      file_meta fm;
	      // std::cout << "Filemeta retrieval" << std::endl;
	      map_fm_client->get(table::FILE_CHUNK_DB, t.source.filename, std::to_string(-1), &fm);

	      std::vector<file> resolve_dts;

	      // Query those datatasks for range
	      // std::cout << "Query dts for range" << std::endl;
	      int *range_bound = (int *)malloc(t.source.size * sizeof(int));
	      range_bound[0] = 0; //tsk[task_idx]->source.offset;
	      // std::cout << "Populate range bound" << std::endl;
	      for (int i = 1; i < t.source.size; ++i) {
		range_bound[i] = range_bound[i-1] + 1;
	      }
	      // std::cout << "Range requests" << std::endl;
	      int range_lower = 0; //tsk[task_idx]->source.offset;
	      int range_upper = t.source.size; // tsk[task_idx]->source.offset + 
	      bool range_resolved = false;
	      file *curr_chunk;
	      for (int i = 0; i < fm.num_chunks; ++i) {
		// std::cout << "i is " << i << std::endl;
		// std::cout << "Check 1" << std::endl;
		if (fm.current_chunk_index - i - 1 >= 0) {
		  // std::cout << "Condition A " << fm.current_chunk_index << std::endl;
		  curr_chunk = &(fm.chunks[fm.current_chunk_index - i - 1].actual_user_chunk);
      
		  // std::cout << "Check 2" << std::endl;
		  if (dtio_system::getInstance(LIB)->range_resolve(&range_bound, t.source.size, range_lower, range_upper, curr_chunk->offset, curr_chunk->offset + curr_chunk->size, &range_resolved)) {
		    // std::cout << "Push" << std::endl;
		    resolve_dts.push_back(*curr_chunk);
		  }
		  if (range_resolved) {
		    break;
		  }
		}
		else {
		  // std::cout << "Condition B" << std::endl;
		  curr_chunk = &(fm.chunks[CHUNK_LIMIT + fm.current_chunk_index - i - 1].actual_user_chunk);
		  // std::cout << "Check 2" << std::endl;
		  if (dtio_system::getInstance(LIB)->range_resolve(&range_bound, t.source.size, range_lower, range_upper, curr_chunk->offset, curr_chunk->offset + curr_chunk->size, &range_resolved)) {
		    // std::cout << "Push" << std::endl;
		    resolve_dts.push_back(*curr_chunk);
		  }
		  if (range_resolved) {
		    break;
		  }
		}
	      }

	      // Check if the read can be performed from buffers. To avoid fragmentation, we read from disk when any part of the read buffer isn't in DTIO.
	      // TODO it would be far better to allow some fragmentation, but this requires significant changes to the read code and considerations about split and sieved read
	      // std::cout << "Check if read can be performed from buffers" << std::endl;
	      if (!range_resolved) {
		range_resolved = true;
		// std::cout << "Source size " << tsk[task_idx]->source.size << std::endl;
		// std::cout << "Destination size " << tsk[task_idx]->destination.size << std::endl;
		for (int i = 0; i < t.source.size; i++) {
		  if (range_bound[i] != -1) {
		    DTIO_LOG_INFO("Range not resolved at " << range_bound[i]);
		    range_resolved = false;
		    break;
		  }
		}
	      }

	      free(range_bound);
	      // Range resolved on current tasks lower down

	      // Resolve range on current task
	      if (range_resolved) {
		for (unsigned i = resolve_dts.size(); i-- > 0; ) {
		  // Currently, we're just iterating backwards so newer DTs overwrite older ones.
		  // TODO better way to do this is to resolve the range by precalculating the offsets and sizes that get pulled into the buffer from each DT.
		  map_client->get(DATASPACE_DB, resolve_dts[i].filename, std::to_string(resolve_dts[i].server), (char *)ptr + ptr_pos + resolve_dts[i].offset - 0,
				  t.source.size - resolve_dts[i].offset + 0); // 0 should be tsk[task_idx]->source.offset
		  // Make sure we get only the size number of elements, and start from the correct offset that is achieved by the DT.
		}
		// map_client->put(DATASPACE_DB, t.source.filename, char_data, datasize,
		// 		std::to_string(t.destination.server));
		// std::cout << static_cast<char *>(data) << std::endl;
		// free(data);
	      }

	      // data_m->get (DATASPACE_DB, t.source.filename,
	      // 		   std::to_string (t.destination.server), char_data);

	      // std::cout << "Get data into " << ptr_pos << " from " << t.source.offset << " size " << t.source.size << std::endl;

	      // strncpy ((char *)buf + ptr_pos, char_data,
	      // 	       t.source.size); // + t.source.offset

	      // free(char_data);
	      // data_m->remove (DATASPACE_DB, t.source.filename,
	      // 		      std::to_string (t.destination.server));

	      size_read += t.destination.size;
	      break;
	    }
	case CACHE:
	  {
	    DTIO_LOG_TRACE("Cache" << std::endl);
	    // data = data_m->get (DATASPACE_DB, t.source.filename,
	    //                     std::to_string (t.destination.server));
	    data_m->get(DATASPACE_DB, t.source.filename,
			std::to_string (t.destination.server), char_data);
	    DTIO_LOG_TRACE("Doing memcpy " << ptr_pos << " " << t.source.offset << " " << t.destination.size << " " << t.source.size << std::endl);
	    memcpy ((char *)ptr + ptr_pos, char_data + (t.source.offset % t.source.size),
		    t.destination.size); // Might not actually be necessary
	    DTIO_LOG_TRACE("Memcpy finished" << std::endl);
	    size_read += t.destination.size;
	    break;
	  }
	}
      free(char_data);
    }
#endif

    ptr_pos += t.destination.size;
  }
  // mdm->update_read_task_info(tasks, filename);
  return size_read;
}

size_t dtio::stdio::fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
  DTIO_LOG_DEBUG ("[STDIO] fwrite entered");
  auto mdm = metadata_manager::getInstance(LIB);
  auto client_queue =
      dtio_system::getInstance(LIB)->get_client_queue(CLIENT_TASK_SUBJECT);
  auto map_client = dtio_system::getInstance(LIB)->map_client();
  auto map_server = dtio_system::getInstance(LIB)->map_server();
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance(LIB);
  auto filename = mdm->get_filename(stream);
  if (!mdm->is_opened(filename))
    throw std::runtime_error("dtio::fwrite() file not opened!");
  file_stat st = mdm->get_stat(filename);
  auto offset = st.file_pointer;
  auto source = file();
  source.offset = 0; // buffer offset, NOT file offset
  source.size = size * count;
  auto destination = file(filename, offset, size * count);

  auto w_task = task(task_type::WRITE_TASK, source, destination);

  w_task.iface = io_client_type::STDIO;
#ifdef TIMERTB
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  auto write_tasks = task_m->build_write_task(w_task, static_cast<const char *>(ptr));
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_write_task()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  DTIO_LOG_TRACE(stream1.str ());
#endif

  const char *write_data_char = static_cast<const char *>(ptr);
  std::string write_data (static_cast<const char *> (ptr), count);

  int index = 0;

  /* Note: We cannot assume buf is null-terminated. It would likely be
   * better to get rid of the string entirely, but for now if we copy
   * with count then it will not assume a null-terminated C string */
  std::vector<std::pair<std::string, std::string>> task_ids =
      std::vector<std::pair<std::string, std::string>>();
  for (auto tsk : write_tasks)
    {
      if (tsk->addDataspace)
	{
	  if (write_data.length () >= tsk->source.offset + tsk->source.size)
	    {
	      auto data
		= write_data.substr (tsk->source.offset, tsk->source.size);
	      DTIO_LOG_TRACE("Write v1 " << write_data.length() << " " << data.length() << " " << tsk->source.offset << " " << tsk->source.size << std::endl);
	      DTIO_LOG_TRACE("WRITE Count " << count << std::endl);
	      data_m->put (DATASPACE_DB, tsk->source.filename, write_data_char, count,
			   std::to_string (tsk->destination.server));
	  
	      // data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
	      //              std::to_string (tsk->destination.server));
	    }
	  else
	    {
	      DTIO_LOG_TRACE("Write v2 " << write_data.length() << " " << tsk->source.size << " " << write_tasks.size() << std::endl);
	      // data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
	      //              std::to_string (tsk->destination.server));
	      DTIO_LOG_TRACE("WRITE Count " << count << std::endl);
	      data_m->put (DATASPACE_DB, tsk->source.filename, write_data_char, count,
			   std::to_string (tsk->destination.server));
	    }
	}

      if (tsk->publish)
        {
          // if (count < tsk->source.size) {
          //   mdm->update_write_task_info (*tsk, tsk->destination.filename, count);
	  // }
          // else {
          //   mdm->update_write_task_info (*tsk, tsk->destination.filename, tsk->source.size);
	  // }
          client_queue->publish_task (tsk);
          task_ids.emplace_back (std::make_pair (
						 std::to_string(tsk->task_id), std::to_string (tsk->destination.server)));
        }
      else
        {
          mdm->update_write_task_info (*tsk, tsk->destination.filename, tsk->source.size);
        }

      index++;
      delete tsk;
    }
  // {
  //   if (tsk->addDataspace) {
  //     if (write_data.length() >= tsk->source.offset + tsk->source.size) {
  //       auto data = write_data.substr(tsk->source.offset, tsk->source.size);
  //       data_m->put(DATASPACE_DB, tsk->source.filename, data,
  //                   std::to_string(tsk->destination.server));
  //     } else {
  //       data_m->put(DATASPACE_DB, tsk->source.filename, write_data,
  //                   std::to_string(tsk->destination.server));
  //     }
  //   }
  //   if (tsk->publish) {
  //     if (size * count < tsk->source.size)
  //       mdm->update_write_task_info(*tsk, filename, size * count);
  //     else
  //       mdm->update_write_task_info(*tsk, filename, tsk->source.size);
  //     client_queue->publish_task(tsk);
  //     task_ids.emplace_back(
  //         std::make_pair(tsk->source.filename,
  //                        std::to_string(tsk->destination.server)));
  //   } else {
  //     mdm->update_write_task_info(*tsk, filename, tsk->source.size);
  //   }

  //   index++;
  //   delete tsk;
  // }


  // NOTE Without this, it's not synchronous. Easy way to add async later
  if (!ConfigManager::get_instance()->ASYNC) {
    for (auto task_id : task_ids) {
      while (!map_server->exists(table::WRITE_FINISHED_DB, task_id.first,
				 std::to_string(-1))) {
      }
      map_server->remove(table::WRITE_FINISHED_DB, task_id.first,
			 std::to_string(-1));
      // map_client->remove(table::DATASPACE_DB, task_id.first, task_id.second);
    }
  }
  return size * count;
}
