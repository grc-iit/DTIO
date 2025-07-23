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

#include "dtio/enumerations.h"
#include "dtio/logger.h"
#include <dtio/drivers/hdf5.h>
#include <dtio/return_codes.h>
#include <dtio/task_builder/task_builder.h>
#include <fcntl.h>
#include <hcl/debug.h>
#include <hdf5.h>
#include <iomanip>
#include <string>
#include <sys/stat.h>
#include <zconf.h>

int
dtio::hdf5::DTIO_Init ()
{
  ConfigManager::get_instance ()->LoadConfig ();
  std::stringstream ss;

  dtio_system::getInstance (service::LIB);
  return 0;
}

int
dtio::hdf5::DTIO_open (const char *filename, const char *dsetname,
                       unsigned flags, bool create, bool hdf5file)
{
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  int myflags = 0;
  char *path;
  bool publish_task = false;
  myflags |= O_RDWR;
  if (hdf5file && (flags & H5F_ACC_TRUNC))
    {
      myflags |= O_TRUNC;
    }
  else
    {
      myflags |= O_APPEND;
    }
  if (hdf5file)
    {
      myflags |= O_DIRECTORY;
    }
  if (dsetname == NULL)
    {
      path = (char *)calloc (strlen (filename) + 1, sizeof (char));
      strcpy (path, filename);
    }
  else
    {
      path = (char *)calloc (strlen (filename) + strlen (dsetname) + 2,
                             sizeof (char));
      sprintf (path, "%s/%s", filename,
               dsetname); // NOTE path separator must be /
    }

  if (!mdm->is_created (path))
    {
      if (!create)
        {
          int existing_size = 0;
          if (ConfigManager::get_instance ()->CHECKFS)
            {
              struct stat st;
              if (stat (path, &st) == 0)
                {
                  file_exists_in_fs = true;
                  existing_size = st.st_size;
                }
            }
          if (!file_exists_in_fs)
            {
              return -1;
            }
          else
            {
              if (mdm->update_on_open (path, myflags, 0, &fd, existing_size)
                  != SUCCESS)
                {
                  throw std::runtime_error (
                      "dtio::posix::open() update failed!");
                }
              publish_task = true;
            }
        }
      else
        {
          if (mdm->create (path, myflags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
          publish_task = true;
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
          publish_task = true;
        }
      else
        return -1;
    }

  if (hdf5file)
    {
      char *filepath = (strncmp (path, "dtio://", 7) == 0) ? (path + 7) : path;
      mkdir (filepath, 0775);
    }
  free (path);
  if (publish_task && ConfigManager::get_instance ()->WORKER_STAGING_SIZE != 0)
    {
      // Publish staging task
      auto f = file (std::string (filename), 0, 0);
      auto s_task = task (task_type::STAGING_TASK, f);
      //   auto client_queue = dtio_system::getInstance (LIB)->get_client_queue
      //   (
      //       CLIENT_TASK_SUBJECT);
      // client_queue->publish_task (&s_task);
    }

  return fd;
}

int
dtio::hdf5::DTIO_close (int fd)
{
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

int
dtio::hdf5::DTIO_write (const char *filename, const char *dsetname,
                        const char *buf, size_t len, uint64_t offset)
{
  char *path;
  if (dsetname == NULL)
    {
      path = (char *)calloc (strlen (filename) + 1, sizeof (char));
      strcpy (path, filename);
    }
  else
    {
      path = (char *)calloc (strlen (filename) + strlen (dsetname) + 2,
                             sizeof (char));
      sprintf (path, "%s/%s", filename,
               dsetname); // NOTE path separator must be /
    }

  auto mdm = metadata_manager::getInstance (LIB);
  //   auto client_queue
  //       = dtio_system::getInstance (LIB)->get_client_queue
  //       (CLIENT_TASK_SUBJECT);
  auto map_client = dtio_system::getInstance (LIB)->map_client ();
  auto map_server = dtio_system::getInstance (LIB)->map_server ();
  auto task_m = dtio_system::getInstance (LIB)->task_composer ();
  auto data_m = data_manager::getInstance (LIB);
  /* auto filename = mdm->get_filename (fd); */
  // auto offset = mdm->get_fp (path);

  /* if (!mdm->is_opened (path)) */
  /*   throw std::runtime_error ("dtio::write() file not opened!"); */
  auto source = file ();
  source.offset = 0; // buffer offset, NOT object offset
  source.size = len;
  auto destination = file (path, offset, len);
  auto w_task = task (task_type::WRITE_TASK, source, destination);

  w_task.iface = io_client_type::HDF5; // io_client_type::POSIX;

  auto write_tasks
      = task_m->build_write_task (w_task, static_cast<const char *> (buf));

  const char *write_data_char = static_cast<const char *> (buf);
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
              DTIO_LOG_TRACE ("Write v1 " << write_data.length () << " "
                                          << data.length () << " "
                                          << tsk->source.offset << " "
                                          << tsk->source.size << std::endl);
              DTIO_LOG_TRACE ("WRITE Count " << len << std::endl);
              data_m->put (DATASPACE_DB, tsk->source.filename, write_data_char,
                           len, std::to_string (tsk->destination.server));

              // data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
              //              std::to_string (tsk->destination.server));
            }
          else
            {
              DTIO_LOG_TRACE ("Write v2 " << write_data.length () << " "
                                          << tsk->source.size << " "
                                          << write_tasks.size () << std::endl);
              // data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
              //              std::to_string (tsk->destination.server));
              DTIO_LOG_TRACE ("WRITE Count " << len << std::endl);
              data_m->put (DATASPACE_DB, tsk->source.filename, write_data_char,
                           len, std::to_string (tsk->destination.server));
            }
        }

      if (tsk->publish)
        {
          if (len < tsk->source.size)
            {
              mdm->update_write_task_info (*tsk, tsk->destination.filename,
                                           len);
            }
          else
            {
              mdm->update_write_task_info (*tsk, tsk->destination.filename,
                                           tsk->source.size);
            }
          // client_queue->publish_task (tsk);

          task_ids.emplace_back (
              std::make_pair (std::to_string (tsk->task_id),
                              std::to_string (tsk->destination.server)));
        }
      else
        {
          mdm->update_write_task_info (*tsk, tsk->destination.filename,
                                       tsk->source.size);
        }

      index++;
      delete tsk;
    }
  // NOTE Without this, it's not synchronous. Easy way to add async later
  if (!ConfigManager::get_instance ()->ASYNC)
    {
      for (auto task_id : task_ids)
        {
          while (!map_server->exists (table::WRITE_FINISHED_DB, task_id.first,
                                      std::to_string (-1)))
            {
            }
          map_server->remove (table::WRITE_FINISHED_DB, task_id.first,
                              std::to_string (-1));
          // map_client->remove (table::DATASPACE_DB, task_id.first,
          // task_id.second);
        }
    }
}

int
dtio::hdf5::DTIO_read (const char *filename, const char *dsetname, char *buf,
                       size_t len, uint64_t off)
{
  char *path;
  if (dsetname == NULL)
    {
      path = (char *)calloc (strlen (filename) + 1, sizeof (char));
      strcpy (path, filename);
    }
  else
    {
      path = (char *)calloc (strlen (filename) + strlen (dsetname) + 2,
                             sizeof (char));
      sprintf (path, "%s/%s", filename,
               dsetname); // NOTE path separator must be /
    }

  auto mdm = metadata_manager::getInstance (LIB);
  //   auto client_queue
  //       = dtio_system::getInstance (LIB)->get_client_queue
  //       (CLIENT_TASK_SUBJECT);
  auto task_m = dtio_system::getInstance (LIB)->task_composer ();
  auto data_m = data_manager::getInstance (LIB);
  // auto filename = mdm->get_filename (fd);
  auto offset = off; // mdm->get_fp (path);
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
  auto r_task = task (task_type::READ_TASK, file (path, offset, len), file ());
  r_task.check_fs = check_fs;
  r_task.iface = io_client_type::POSIX;
#ifdef TIMERTB
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  auto tasks = task_m->build_read_task (r_task);
  DTIO_LOG_TRACE ("Task len " << tasks.size () << std::endl);
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_read_task()," << std::fixed << std::setprecision (10)
          << t.pauseTime () << "\n";
  DTIO_LOG_TRACE (stream1.str ());
#endif
  int ptr_pos = 0;
  size_t size_read = 0;
  task *ttp;
  for (auto t : tasks)
    {
      std::string data;
#if (STACK_ALLOCATION)
      {
        // char char_data[MAX_IO_UNIT];
        char *char_data;
        char_data = (char *)malloc (MAX_IO_UNIT);
        switch (t.source.location)
          {
          case PFS:
            {
              DTIO_LOG_DEBUG ("in pfs");
              if (ConfigManager::get_instance ()->CHECKFS)
                {
                  hcl::Timer timer = hcl::Timer ();
                  // client_queue->publish_task (&t);

                  while (
                      !data_m->exists (DATASPACE_DB, t.source.filename,
                                       std::to_string (t.destination.server)))
                    {
                    }

                  data_m->get (DATASPACE_DB, t.source.filename,
                               std::to_string (t.destination.server),
                               char_data);

                  strncpy ((char *)buf + ptr_pos, char_data + t.source.offset,
                           t.destination.size);
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
              // client_queue->publish_task (&t);
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
              DTIO_LOG_TRACE ("Cache" << std::endl);
              // data = data_m->get (DATASPACE_DB, t.source.filename,
              //                     std::to_string (t.destination.server));
              data_m->get (DATASPACE_DB, t.source.filename,
                           std::to_string (t.destination.server), char_data);
              DTIO_LOG_TRACE ("Doing memcpy "
                              << ptr_pos << " " << t.source.offset << " "
                              << t.destination.size << " " << t.source.size
                              << std::endl);
              memcpy ((char *)buf + ptr_pos,
                      char_data + (t.source.offset % t.source.size),
                      t.destination.size); // Might not actually be necessary
              DTIO_LOG_TRACE ("Memcpy finished" << std::endl);
              size_read += t.destination.size;
              break;
            }
          }
        free (char_data);
      }
#else
      {
        char *char_data = (char *)malloc (MAX_IO_UNIT);
        switch (t.source.location)
          {
          case PFS:
            {
              DTIO_LOG_DEBUG ("in pfs");
              if (ConfigManager::get_instance ()->CHECKFS)
                {
                  hcl::Timer timer = hcl::Timer ();
                  // client_queue->publish_task (&t);

                  while (
                      !data_m->exists (DATASPACE_DB, t.source.filename,
                                       std::to_string (t.destination.server)))
                    {
                    }

                  data_m->get (DATASPACE_DB, t.source.filename,
                               std::to_string (t.destination.server),
                               char_data);

                  strncpy ((char *)buf + ptr_pos, char_data + t.source.offset,
                           t.destination.size);
                  free (char_data);
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
              // hcl::Timer timer = hcl::Timer ();
              // // client_queue->publish_task (&t);
              // while (!data_m->exists (DATASPACE_DB, t.source.filename,
              // 			      std::to_string
              // (t.destination.server)))
              // 	{
              // 	  // std::cerr<<"looping\n";
              // 	}

              auto map_client
                  = dtio_system::getInstance (WORKER)->map_client ();
              auto map_fm_client
                  = dtio_system::getInstance (WORKER)->map_client (
                      "metadata+filemeta");
              std::vector<file> resolve_dts;

              // Query the metadata maps to get the datatasks associated with a
              // file
              file_meta fm;
              // std::cout << "Filemeta retrieval" << std::endl;
              map_fm_client->get (table::FILE_CHUNK_DB, t.source.filename,
                                  std::to_string (-1), &fm);

              // Query those datatasks for range
              // std::cout << "Query dts for range" << std::endl;
              int *range_bound = (int *)malloc (t.source.size * sizeof (int));
              range_bound[0] = 0; // tsk[task_idx]->source.offset;
              // std::cout << "Populate range bound" << std::endl;
              for (int i = 1; i < t.source.size; ++i)
                {
                  range_bound[i] = range_bound[i - 1] + 1;
                }
              // std::cout << "Range requests" << std::endl;
              int range_lower = 0; // tsk[task_idx]->source.offset;
              int range_upper
                  = t.source.size; // tsk[task_idx]->source.offset +
              bool range_resolved = false;
              file *curr_chunk;
              for (int i = 0; i < fm.num_chunks; ++i)
                {
                  // std::cout << "i is " << i << std::endl;
                  // std::cout << "Check 1" << std::endl;
                  if (fm.current_chunk_index - i - 1 >= 0)
                    {
                      // std::cout << "Condition A " << fm.current_chunk_index
                      // << std::endl;
                      curr_chunk = &(fm.chunks[fm.current_chunk_index - i - 1]
                                         .actual_user_chunk);

                      // std::cout << "Check 2" << std::endl;
                      if (dtio_system::getInstance (WORKER)->range_resolve (
                              &range_bound, t.source.size, range_lower,
                              range_upper, curr_chunk->offset,
                              curr_chunk->offset + curr_chunk->size,
                              &range_resolved))
                        {
                          // std::cout << "Push" << std::endl;
                          resolve_dts.push_back (*curr_chunk);
                        }
                      if (range_resolved)
                        {
                          break;
                        }
                    }
                  else
                    {
                      // std::cout << "Condition B" << std::endl;
                      curr_chunk
                          = &(fm.chunks[CHUNK_LIMIT + fm.current_chunk_index
                                        - i - 1]
                                  .actual_user_chunk);
                      // std::cout << "Check 2" << std::endl;
                      if (dtio_system::getInstance (WORKER)->range_resolve (
                              &range_bound, t.source.size, range_lower,
                              range_upper, curr_chunk->offset,
                              curr_chunk->offset + curr_chunk->size,
                              &range_resolved))
                        {
                          // std::cout << "Push" << std::endl;
                          resolve_dts.push_back (*curr_chunk);
                        }
                      if (range_resolved)
                        {
                          break;
                        }
                    }
                }

              // Check if the read can be performed from buffers. To avoid
              // fragmentation, we read from disk when any part of the read
              // buffer isn't in DTIO.
              // TODO it would be far better to allow some fragmentation, but
              // this requires significant changes to the read code and
              // considerations about split and sieved read std::cout << "Check
              // if read can be performed from buffers" << std::endl;
              if (!range_resolved)
                {
                  range_resolved = true;
                  // std::cout << "Source size " << tsk[task_idx]->source.size
                  // << std::endl; std::cout << "Destination size " <<
                  // tsk[task_idx]->destination.size << std::endl;
                  for (int i = 0; i < t.source.size; i++)
                    {
                      if (range_bound[i] != -1)
                        {
                          DTIO_LOG_INFO ("Range not resolved at "
                                         << range_bound[i])
                          range_resolved = false;
                          break;
                        }
                    }
                }

              free (range_bound);
              // Range resolved on current tasks lower down

              // Resolve range on current task
              if (range_resolved)
                {
                  for (unsigned i = resolve_dts.size (); i-- > 0;)
                    {
                      // Currently, we're just iterating backwards so newer DTs
                      // overwrite older ones.
                      // TODO better way to do this is to resolve the range by
                      // precalculating the offsets and sizes that get pulled
                      // into the buffer from each DT.
                      map_client->get (
                          DATASPACE_DB, resolve_dts[i].filename,
                          std::to_string (resolve_dts[i].server),
                          char_data + resolve_dts[i].offset - 0,
                          t.source.size - resolve_dts[i].offset
                              + 0); // 0 should be tsk[task_idx]->source.offset
                      // Make sure we get only the size number of elements, and
                      // start from the correct offset that is achieved by the
                      // DT.
                    }
                  map_client->put (DATASPACE_DB, t.source.filename, char_data,
                                   MAX_IO_UNIT,
                                   std::to_string (t.destination.server));
                  free (char_data);
                  continue;
                }

              // data_m->get (DATASPACE_DB, t.source.filename,
              // 		   std::to_string (t.destination.server),
              // char_data);

              // strncpy ((char *)buf + ptr_pos, char_data + t.source.offset,
              // 	       t.destination.size);
              // free(char_data);
              // data_m->remove (DATASPACE_DB, t.source.filename,
              // 		      std::to_string (t.destination.server));

              size_read += t.destination.size;
              break;
            }
          case CACHE:
            {
              DTIO_LOG_TRACE ("Cache" << std::endl);
              // data = data_m->get (DATASPACE_DB, t.source.filename,
              //                     std::to_string (t.destination.server));
              data_m->get (DATASPACE_DB, t.source.filename,
                           std::to_string (t.destination.server), char_data);
              DTIO_LOG_TRACE ("Doing memcpy "
                              << ptr_pos << " " << t.source.offset << " "
                              << t.destination.size << " " << t.source.size
                              << std::endl);
              memcpy ((char *)buf + ptr_pos,
                      char_data + (t.source.offset % t.source.size),
                      t.destination.size); // Might not actually be necessary
              DTIO_LOG_TRACE ("Memcpy finished" << std::endl);
              free (char_data);
              size_read += t.destination.size;
              break;
            }
          }
      }
#endif

      ptr_pos += t.destination.size;
    }
  // mdm->update_read_task_info (tasks, path);
  return size_read;
}
