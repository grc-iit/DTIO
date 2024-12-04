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

#include "dtio/common/enumerations.h"
#include "dtio/common/logger.h"
#include <dtio/common/return_codes.h>
#include <dtio/common/task_builder/task_builder.h>
#include <dtio/drivers/hdf5.h>
#include <fcntl.h>
#include <hcl/common/debug.h>
#include <iomanip>
#include <sys/stat.h>
#include <zconf.h>
#include <string>
#include <hdf5.h>

int dtio::hdf5::DTIO_Init()
{
  ConfigManager::get_instance ()->LoadConfig ();
  std::stringstream ss;

  dtio_system::getInstance (service::LIB);
  return 0;
}

int dtio::hdf5::DTIO_open(const char *filename, const char *dsetname, unsigned flags, bool create, bool hdf5file) {
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  int myflags = 0;
  char *path;
  myflags |= O_RDWR;
  if (hdf5file && (flags & H5F_ACC_TRUNC)) {
    myflags |= O_TRUNC;
  }
  else {
    myflags |= O_APPEND;
  }
  if (hdf5file) {
    myflags |= O_DIRECTORY;
  }
  if (dsetname == NULL) {
    path = (char *)calloc(strlen(filename) + 1, sizeof(char));
    strcpy(path, filename);
  }
  else {
    path = (char *)calloc(strlen(filename) + strlen(dsetname) + 2, sizeof(char));
    sprintf(path, "%s/%s", filename, dsetname); // NOTE path separator must be /
  }

  if (!mdm->is_created (path))
    {
      if (!create)
        {
          if (ConfigManager::get_instance ()->CHECKFS)
            {
              struct stat st;
              if (stat (path, &st) == 0)
                {
                  file_exists_in_fs = true;
                }
            }
          if (!file_exists_in_fs)
            {
              return -1;
            }
          else
            {
              if (mdm->update_on_open (path, myflags, 0, &fd) != SUCCESS)
                {
                  throw std::runtime_error (
                      "dtio::posix::open() update failed!");
                }
            }
        }
      else
        {
          if (mdm->create (path, myflags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
        }
    }
  else
    {
      if (!mdm->is_opened (path))
        {
          if (mdm->update_on_open (path, myflags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() update failed!");
            }
        }
      else
        return -1;
    }

  if (hdf5file) {
    char *filepath = (strncmp(path, "dtio://", 7) == 0) ? (path + 7) : path;
    mkdir(filepath, 0775);
  }
  free(path);
  return fd;
}

int dtio::hdf5::DTIO_close(int fd) {
  auto mdm = metadata_manager::getInstance (LIB);
  if (!mdm->is_opened (fd))
    {
      return LIB__FCLOSE_FAILED;
    }
  if (mdm->update_on_close (fd) != SUCCESS)
    {
      return LIB__FCLOSE_FAILED;
    }
  return SUCCESS;
}

int dtio::hdf5::DTIO_write(const char *filename, const char *dsetname, const char *buf, size_t len, uint64_t offset) {
    char *path;
    if (dsetname == NULL) {
      path = (char *)calloc(strlen(filename) + 1, sizeof(char));
      strcpy(path, filename);
    }
    else {
      path = (char *)calloc(strlen(filename) + strlen(dsetname) + 2, sizeof(char));
      sprintf(path, "%s/%s", filename, dsetname); // NOTE path separator must be /
    }

    auto mdm = metadata_manager::getInstance (LIB);
    auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
    auto map_client = dtio_system::getInstance (LIB)->map_client ();
    auto map_server = dtio_system::getInstance (LIB)->map_server ();
    auto task_m = dtio_system::getInstance(LIB)->task_composer();
    auto data_m = data_manager::getInstance (LIB);
    /* auto filename = mdm->get_filename (fd); */
    // auto offset = mdm->get_fp (path);

    /* if (!mdm->is_opened (path)) */
    /*   throw std::runtime_error ("dtio::write() file not opened!"); */
    auto source = file();
    source.offset = 0; // buffer offset, NOT object offset
    source.size = len;
    auto destination = file(path, offset, len);
    auto w_task
      = task (task_type::WRITE_TASK, source, destination);

    w_task.iface = io_client_type::POSIX;

    auto write_tasks
      = task_m->build_write_task (w_task, static_cast<const char *> (buf));

    const char *write_data_char = static_cast<const char *>(buf);
    std::string write_data (static_cast<const char *> (buf), len);

  int index = 0;
  /* Note: We cannot assume buf is null-terminated. It would likely be
   * better to get rid of the string entirely, but for now if we copy
   * with count then it will not assume a null-terminated C string */
  std::vector<std::pair<std::string, std::string> > task_ids
      = std::vector<std::pair<std::string, std::string> > ();

  for (auto tsk : write_tasks)
    {
      if (tsk->addDataspace)
	{
	  if (write_data.length () >= tsk->source.offset + tsk->source.size)
	    {
	      auto data
		= write_data.substr (tsk->source.offset, tsk->source.size);
	      DTIO_LOG_TRACE("Write v1 " << write_data.length() << " " << data.length() << " " << tsk->source.offset << " " << tsk->source.size << std::endl);
	      DTIO_LOG_TRACE("WRITE Count " << len << std::endl);
	      data_m->put (DATASPACE_DB, tsk->source.filename, write_data_char, len,
			   std::to_string (tsk->destination.server));

	      // data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
	      //              std::to_string (tsk->destination.server));
	    }
	  else
	    {
	      DTIO_LOG_TRACE("Write v2 " << write_data.length() << " " << tsk->source.size << " " << write_tasks.size() << std::endl);
	      // data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
	      //              std::to_string (tsk->destination.server));
	      DTIO_LOG_TRACE("WRITE Count " << len << std::endl);
	      data_m->put (DATASPACE_DB, tsk->source.filename, write_data_char, len,
			   std::to_string (tsk->destination.server));
	    }
	}

      if (tsk->publish)
        {
          if (len < tsk->source.size) {
            mdm->update_write_task_info (*tsk, tsk->destination.filename, len);
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
  // NOTE Without this, it's not synchronous. Easy way to add async later
  if (!ConfigManager::get_instance ()->ASYNC) {
    for (auto task_id : task_ids)
      {
	while (!map_server->exists (table::WRITE_FINISHED_DB, task_id.first,
				    std::to_string (-1)))
	  {
	  }
	map_server->remove (table::WRITE_FINISHED_DB, task_id.first,
			    std::to_string (-1));
	// map_client->remove (table::DATASPACE_DB, task_id.first, task_id.second);
      }
  }
}

int dtio::hdf5::DTIO_read(const char *filename, const char *dsetname, char *buf, size_t len, uint64_t off) {
  char *path;
  if (dsetname == NULL) {
    path = (char *)calloc(strlen(filename) + 1, sizeof(char));
    strcpy(path, filename);
  }
  else {
    path = (char *)calloc(strlen(filename) + strlen(dsetname) + 2, sizeof(char));
    sprintf(path, "%s/%s", filename, dsetname); // NOTE path separator must be /
  }

  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance (LIB);
  // auto filename = mdm->get_filename (fd);
  auto offset = off; //mdm->get_fp (path);
  bool check_fs = false;
  task *task_i;
  task_i = nullptr;

  if (!mdm->is_opened (path))
    {
      if (!ConfigManager::get_instance ()->CHECKFS)
        {
          return 0;
        }
      if (!mdm->is_created (path))
        {
          check_fs = true;
        }
    }
  auto r_task
      = task (task_type::READ_TASK, file (path, offset, len), file ());
  r_task.check_fs = check_fs;
  r_task.iface = io_client_type::POSIX;
#ifdef TIMERTB
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  auto tasks = task_m->build_read_task (r_task);
  DTIO_LOG_TRACE("Task len " << tasks.size() << std::endl);
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_read_task()," << std::fixed << std::setprecision (10)
          << t.pauseTime () << "\n";
  DTIO_LOG_TRACE(stream1.str ());
#endif
  int ptr_pos = 0;
  size_t size_read = 0;
  task *ttp;
  for (auto t : tasks)
    {
      std::string data;
#if(STACK_ALLOCATION)
      {
	// char char_data[MAX_IO_UNIT];
	char *char_data;
	char_data = (char *)malloc(MAX_IO_UNIT);
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

		  strncpy ((char *)buf + ptr_pos,
			   char_data + t.source.offset, t.destination.size);
		  auto print_test = std::string ((char *)buf);

		  data_m->remove (DATASPACE_DB, t.source.filename,
				  std::to_string (t.destination.server));

		  size_read += t.source.size;
		}
	      else
		{
		  DTIO_LOG_ERROR ("in pfs no checkfs");
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

	      strncpy ((char *)buf + ptr_pos, char_data + t.source.offset,
		       t.destination.size);

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
	      memcpy ((char *)buf + ptr_pos, char_data + (t.source.offset % t.source.size),
		      t.destination.size); // Might not actually be necessary
	      DTIO_LOG_TRACE("Memcpy finished" << std::endl);
	      size_read += t.destination.size;
	      break;
	    }
	  }
	free(char_data);
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

		  strncpy ((char *)buf + ptr_pos,
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

	      strncpy ((char *)buf + ptr_pos, char_data + t.source.offset,
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
	      memcpy ((char *)buf + ptr_pos, char_data + (t.source.offset % t.source.size),
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
  mdm->update_read_task_info (tasks, path);
  return size_read;
}
