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
  if (!mdm->is_created(filename)) {
    if (strcmp(mode, "a") == 0 && strcmp(mode, "w") == 0) {
      if (ConfigManager::get_instance ()->CHECKFS)
	{
	  struct stat st;
	  if (stat (filename, &st) == 0)
	    {
	      file_exists_in_fs = true;
	    }
	}
      if (!file_exists_in_fs)
	{
	  return nullptr;
	}
      else
	{
	  if (mdm->update_on_open (filename, mode, fh) != SUCCESS)
	    {
	      throw std::runtime_error ("dtio::fopen() update failed!");
	    }
	}
    } else {
      if (mdm->create(filename, mode, fh) != SUCCESS) {
        throw std::runtime_error("dtio::fopen() create failed!");
      }
    }
  } else {
    if (!mdm->is_opened(filename)) {
      if (mdm->update_on_open(filename, mode, fh) != SUCCESS) {
        throw std::runtime_error("dtio::fopen() update failed!");
      }
    } else {
      return nullptr;
    }
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
  if (mdm->get_mode(filename) == "a" || mdm->get_mode(filename) == "a+")
    return 0;
  auto size = mdm->get_filesize(filename);
  auto fp = mdm->get_fp(filename);
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

size_t dtio::stdio::fread(void *ptr, size_t size, size_t count, FILE *stream) {
  auto mdm = metadata_manager::getInstance(LIB);
  auto client_queue =
      dtio_system::getInstance(LIB)->get_client_queue(CLIENT_TASK_SUBJECT);
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance(LIB);
  auto filename = mdm->get_filename(stream);
  auto offset = mdm->get_fp(filename);
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
  int ptr_pos = 0;
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
		free(char_data);
		// auto print_test = std::string ((char *)buf);

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
	case BUFFERS:
	  {
	    hcl::Timer timer = hcl::Timer ();
	    client_queue->publish_task (&t);
	    while (!data_m->exists (DATASPACE_DB, t.source.filename,
				    std::to_string (t.destination.server)))
	      {
		// std::cerr<<"looping\n";
	      }
	    data_m->get (DATASPACE_DB, t.source.filename,
			 std::to_string (t.destination.server), char_data);

	    strncpy ((char *)ptr + ptr_pos, char_data + t.source.offset,
		     t.destination.size);
	    free(char_data);
	    data_m->remove (DATASPACE_DB, t.source.filename,
			    std::to_string (t.destination.server));

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
	    free(char_data);
	    size_read += t.destination.size;
	    break;
	  }
	}
    }
#endif

    ptr_pos += t.destination.size;
  }
  mdm->update_read_task_info(tasks, filename);
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
  auto offset = mdm->get_fp(filename);
  if (!mdm->is_opened(filename))
    throw std::runtime_error("dtio::fwrite() file not opened!");
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
          if (count < tsk->source.size) {
            mdm->update_write_task_info (*tsk, tsk->destination.filename, count);
	  }
          else {
            mdm->update_write_task_info (*tsk, tsk->destination.filename, tsk->source.size);
	  }
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
  for (auto task_id : task_ids) {
    while (!map_server->exists(table::WRITE_FINISHED_DB, task_id.first,
                               std::to_string(-1))) {
    }
    map_server->remove(table::WRITE_FINISHED_DB, task_id.first,
                       std::to_string(-1));
    // map_client->remove(table::DATASPACE_DB, task_id.first, task_id.second);
  }
  return size * count;
}
