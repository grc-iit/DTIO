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

#include "dtio/common/enumerations.h"
#include "dtio/common/logger.h"
#include <dtio/common/return_codes.h>
#include <dtio/common/task_builder/task_builder.h>
#include <dtio/drivers/posix.h>
#include <fcntl.h>
#include <hcl/common/debug.h>
#include <iomanip>
#include <sys/stat.h>
#include <zconf.h>
// #include <adapter/posix/posix_api.h>

// #include <filesystem>

// namespace stdfs = std::filesystem;
// std::shared_ptr<dtio::posix::PosixApi> dtio::posix::PosixApi::instance =
// nullptr;

int
dtio::posix::open (const char *filename, int flags)
{
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  if (!mdm->is_created (filename))
    {
      if (!(flags & O_CREAT))
        {
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
              return -1;
            }
          else
            {
              if (mdm->update_on_open (filename, flags, 0, &fd) != SUCCESS)
                {
                  throw std::runtime_error (
                      "dtio::posix::open() update failed!");
                }
            }
        }
      else
        {
          if (mdm->create (filename, flags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
        }
    }
  else
    {
      if (!mdm->is_opened (filename))
        {
          if (mdm->update_on_open (filename, flags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() update failed!");
            }
        }
      else
        return -1;
    }
  return fd;
}

int
dtio::posix::open (const char *filename, int flags, mode_t mode)
{
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  if (!mdm->is_created (filename))
    {
      if (!(flags & O_CREAT))
        {
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
              return -1;
            }
          else
            {
              if (mdm->update_on_open (filename, flags, mode, &fd) != SUCCESS)
                {
                  throw std::runtime_error (
                      "dtio::posix::open() update failed!");
                }
            }
        }
      else
        {
          if (mdm->create (filename, flags, mode, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
        }
    }
  else
    {
      if (!mdm->is_opened (filename))
        {
          if (mdm->update_on_open (filename, flags, mode, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() update failed!");
            }
        }
      else
        return -1;
    }
  return fd;
}

int
dtio::posix::open64 (const char *filename, int flags)
{
  DTIO_LOG_DEBUG ("[POSIX] Open64 filename " << filename);
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  if (!mdm->is_created (filename))
    {
      if (!(flags & O_CREAT))
        {
          DTIO_LOG_TRACE ("[POSIX] Open64 file not marked created");
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
              return -1;
            }
          else
            {
              if (mdm->update_on_open (filename, flags, 0, &fd) != SUCCESS)
                {
                  throw std::runtime_error (
                      "dtio::posix::open() update failed!");
                }
            }
        }
      else
        {
          if (mdm->create (filename, flags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
        }
    }
  else
    {
      if (!mdm->is_opened (filename))
        {
          if (mdm->update_on_open (filename, flags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() update failed!");
            }
        }
      else
        {
          DTIO_LOG_DEBUG ("[POSIX] Open64 file already opened");
          return -1;
        }
    }
  return fd;
}

int
dtio::posix::open64 (const char *filename, int flags, mode_t mode)
{
  DTIO_LOG_DEBUG ("[POSIX] Open64 filename " << filename);
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  if (!mdm->is_created (filename))
    {
      if (!(flags & O_CREAT))
        {
          DTIO_LOG_TRACE ("[POSIX] Open64 file not marked created");
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
              return -1;
            }
          else
            {
              if (mdm->update_on_open (filename, flags, mode, &fd) != SUCCESS)
                {
                  throw std::runtime_error (
                      "dtio::posix::open() update failed!");
                }
            }
        }
      else
        {
          if (mdm->create (filename, flags, mode, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
        }
    }
  else
    {
      if (!mdm->is_opened (filename))
        {
          if (mdm->update_on_open (filename, flags, mode, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() update failed!");
            }
        }
      else
        {
          DTIO_LOG_DEBUG ("[POSIX] Open64 file already opened");
          return -1;
        }
    }
  return fd;
}

int
dtio::posix::unlink (const char *pathname)
{
  DTIO_LOG_DEBUG ("[POSIX] unlinking " << pathname);
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto map_client = dtio_system::getInstance (LIB)->map_client ();
  auto map_server = dtio_system::getInstance (LIB)->map_server ();
  auto task_m = task_builder::getInstance (LIB);
  auto data_m = data_manager::getInstance (LIB);
  auto offset = mdm->get_fp (pathname);
  if (!mdm->is_created (pathname))
    {
      throw std::runtime_error ("dtio::posix::unlink() file doesn't exist!");
    }
  auto f = file (std::string (pathname), offset, 0);
  auto d_task = task (task_type::DELETE_TASK, f);
  d_task.iface = io_client_type::POSIX;
  auto delete_tasks = task_m->build_delete_task (d_task);
  int index = 0;
  std::vector<std::pair<std::string, std::string> > task_ids
      = std::vector<std::pair<std::string, std::string> > ();
  for (auto task : delete_tasks)
    {
      if (task.publish)
        {
          mdm->update_delete_task_info (task, pathname);
          client_queue->publish_task (&task);
          task_ids.emplace_back (std::make_pair (
              task.source.filename, std::to_string (task.source.server)));
        }
      else
        {
          mdm->update_delete_task_info (task, pathname);
        }
      index++;
    }
  return 0;
}

int
dtio::posix::fsync (int fd)
{
  // We don't want to do anything for flush
  return 0;
}

int
dtio::posix::rename (const char *oldpath, const char *newpath)
{
  // FIXME not implemented
  return 0;
}

int
dtio::posix::mystat (const char *pathname, struct stat *statbuf)
{
  // Note: I actually had to rename stat in order to use it for file existence
  // checks
  // FIXME right now we just grab file size, which is ok for IOR but also bad
  auto mdm = metadata_manager::getInstance (LIB);
  if (!mdm->is_created (pathname))
    {
      return -1;
    }
  statbuf->st_size = mdm->get_filesize (pathname);
  ;
  DTIO_LOG_TRACE ("[DTIO][POSIX][STAT] " << pathname << " "
                                        << statbuf->st_size);
  return 0;
}

int
dtio::posix::mystat64 (const char *pathname, struct stat64 *statbuf)
{
  // FIXME right now we just grab file size, which is ok for IOR but also bad
  auto mdm = metadata_manager::getInstance (LIB);
  if (!mdm->is_created (pathname))
    {
      return -1;
    }
  statbuf->st_size = mdm->get_filesize (pathname);
  DTIO_LOG_TRACE ("[DTIO][POSIX][STAT] " << pathname << " "
                                        << statbuf->st_size);
  return 0;
}

int
dtio::posix::mknod (const char *pathname, mode_t mode, dev_t dev)
{
  // FIXME not implemented
  return 0;
}

int
dtio::posix::close (int fd)
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

off_t
dtio::posix::lseek (int fd, off_t offset, int whence)
{
  auto mdm = metadata_manager::getInstance (LIB);
  if (!mdm->is_opened(fd)) {
    return EBADF;
  }
  auto filename = mdm->get_filename (fd);
  if (mdm->get_flags (filename) & O_APPEND)
    return 0;
  auto size = mdm->get_filesize (filename);
  auto fp = mdm->get_fp (filename);
  switch (whence)
    {
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
      // break;
    default:
      fprintf (stderr, "Seek origin fault!\n");
      return -1;
    }
  if (!mdm->is_opened (fd))
    return -1;
  return mdm->update_on_seek (filename, static_cast<size_t> (offset),
                              static_cast<size_t> (whence));
}

off_t
dtio::posix::lseek64 (int fd, off_t offset, int whence)
{
  auto mdm = metadata_manager::getInstance (LIB);
  if (!mdm->is_opened(fd)) {
    return EBADF;
  }
  auto filename = mdm->get_filename (fd);
  if (mdm->get_flags (filename) & O_APPEND)
    return 0;

  DTIO_LOG_DEBUG ("[POSIX] Seek filename ", filename);
  auto size = mdm->get_filesize (filename);
  DTIO_LOG_DEBUG ("[POSIX] Seek filesize ", size);
  auto fp = mdm->get_fp (filename);
  switch (whence)
    {
    case SEEK_SET:
      // if (offset > size) {
      // 	fprintf (stderr, "Seek offset greater than size!\n");
      //   return -1;
      // }
      break;
    case SEEK_CUR:
      // fp + offset > size ||
      if (fp + offset < 0)
        {
          fprintf (stderr, "Seek offset out of range!\n");
          return -1;
        }
      break;
    case SEEK_END:
      // if (offset > 0) {
      // 	fprintf (stderr, "Seek offset after end of file!\n");
      //   return -1;
      // }
      break;
    default:
      fprintf (stderr, "Seek origin fault!\n");
      return -1;
    }
  if (!mdm->is_opened (fd))
    return -1;
  return mdm->update_on_seek (filename, static_cast<size_t> (offset),
                              static_cast<size_t> (whence));
}

std::vector<task>
dtio::posix::read_async (int fd, size_t count)
{
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto task_m = task_builder::getInstance (LIB);
  auto filename = mdm->get_filename (fd);
  auto offset = mdm->get_fp (filename);
  if (!mdm->is_opened (filename))
    return std::vector<task> ();
  auto r_task
      = task (task_type::READ_TASK, file (filename, offset, count), file ());
  r_task.iface = io_client_type::POSIX;
#ifdef TIMERTB
  Timer t = Timer ();
  t.resumeTime ();
#endif
  auto tasks = task_m->build_read_task (r_task);
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_read_task()," << std::fixed << std::setprecision (10)
          << t.pauseTime () << "\n";
  std::cout << stream1.str ();
#endif

  for (auto task : tasks)
    {
      std::string data;
      switch (task.source.location)
        {
        case PFS:
          {
            std::cerr << "async in pfs\n";
          }
        case BUFFERS:
          {
            // std::cerr<<"async in buffers\n";
            client_queue->publish_task (&task);
            break;
          }
        case CACHE:
          {
            std::cerr << "async in cache\n";
            break;
          }
        }
    }
  return tasks;
}

std::size_t
read_wait (void *ptr, std::vector<task> &tasks, std::string filename)
{
  auto mdm = metadata_manager::getInstance (LIB);
  auto data_m = data_manager::getInstance (LIB);
  int ptr_pos = 0;
  std::size_t size_read = 0;
  for (auto task : tasks)
    {
      std::string data;
      switch (task.source.location)
        {
        case PFS:
          {
            std::cerr << "wait in pfs\n";
          }
        case BUFFERS:
          {
            // std::cerr<<"wait in buffers\n";
            Timer wait_timer = Timer ();
            int count = 0;
            while (!data_m->exists (DATASPACE_DB, task.destination.filename,
                                    std::to_string (task.destination.server)))
              {
                wait_timer.pauseTime ();
                if (wait_timer.getElapsedTime () > 5 && count % 1000000 == 0)
                  {
                    int rank;
                    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
                    std::stringstream stream;
                    stream << "Waiting for task more than 5 seconds rank:"
                           << rank
                           << "dataspace_id:" << task.destination.filename;
                    count++;
                  }
                wait_timer.resumeTime ();
              }
            data = data_m->get (DATASPACE_DB, task.destination.filename,
                                std::to_string (task.destination.server));

            data_m->remove (DATASPACE_DB, task.destination.filename,
                            std::to_string (task.destination.server));
            memcpy (ptr + ptr_pos, data.c_str () + task.destination.offset,
                    task.destination.size);
            size_read += task.destination.size;
            break;
          }
        case CACHE:
          {
            std::cerr << "wait in cache\n";
            data = data_m->get (DATASPACE_DB, task.source.filename,
                                std::to_string (task.destination.server));
            memcpy (ptr + ptr_pos, data.c_str () + task.source.offset,
                    task.source.size);
            size_read += task.source.size;
            break;
          }
        }
      ptr_pos += task.destination.size;
    }
  mdm->update_read_task_info (tasks, filename);
  return size_read;
}

ssize_t
dtio::posix::read (int fd, void *buf, size_t count)
{
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto task_m = task_builder::getInstance (LIB);
  auto data_m = data_manager::getInstance (LIB);
  auto filename = mdm->get_filename (fd);
  auto offset = mdm->get_fp (filename);
  bool check_fs = false;
  task *task_i;
  task_i = nullptr;

  if (!mdm->is_opened (filename))
    {
      if (!ConfigManager::get_instance ()->CHECKFS)
        {
          return 0;
        }
      if (!mdm->is_created (filename))
        {
          check_fs = true;
        }
    }
  auto r_task
      = task (task_type::READ_TASK, file (filename, offset, count), file ());
  r_task.check_fs = check_fs;
  r_task.iface = io_client_type::POSIX;
#ifdef TIMERTB
  Timer t = Timer ();
  t.resumeTime ();
#endif
  auto tasks = task_m->build_read_task (r_task);
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_read_task()," << std::fixed << std::setprecision (10)
          << t.pauseTime () << "\n";
  std::cout << stream1.str ();
#endif
  int ptr_pos = 0;
  size_t size_read = 0;
  task *ttp;
  for (auto t : tasks)
    {
      std::string data;
      switch (t.source.location)
        {
        case PFS:
          {
            DTIO_LOG_DEBUG ("in pfs");
            if (ConfigManager::get_instance ()->CHECKFS)
              {
                Timer timer = Timer ();
                client_queue->publish_task (&t);

                while (!data_m->exists (DATASPACE_DB, t.source.filename,
                                        std::to_string (t.destination.server)))
                  {
                  }

                data = data_m->get (DATASPACE_DB, t.source.filename,
                                    std::to_string (t.destination.server));

                strncpy ((char *)buf + ptr_pos,
                         data.c_str () + t.source.offset, t.destination.size);
                auto print_test = std::string ((char *)buf);

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
            Timer timer = Timer ();
            client_queue->publish_task (&t);
            while (!data_m->exists (DATASPACE_DB, t.source.filename,
                                    std::to_string (t.destination.server)))
              {
                // std::cerr<<"looping\n";
              }
            data = data_m->get (DATASPACE_DB, t.source.filename,
                                std::to_string (t.destination.server));

            strncpy ((char *)buf + ptr_pos, data.c_str () + t.source.offset,
                     t.destination.size);

            data_m->remove (DATASPACE_DB, t.source.filename,
                            std::to_string (t.destination.server));

            size_read += t.source.size;
            break;
          }
        case CACHE:
          {
            data = data_m->get (DATASPACE_DB, t.source.filename,
                                std::to_string (t.destination.server));
            memcpy (buf + ptr_pos, data.c_str () + t.source.offset,
                    t.source.size);
            size_read += t.source.size;
            break;
          }
        }

      ptr_pos += t.destination.size;
    }
  mdm->update_read_task_info (tasks, filename);
  return size_read;
}

std::vector<task *>
dtio::posix::write_async (int fd, const void *buf, size_t count)
{
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto task_m = task_builder::getInstance (LIB);
  auto data_m = data_manager::getInstance (LIB);
  auto filename = mdm->get_filename (fd);
  auto offset = mdm->get_fp (filename);
  if (!mdm->is_opened (filename))
    throw std::runtime_error ("dtio::posix::write() file not opened!");
  auto w_task
      = task (task_type::WRITE_TASK, file (filename, offset, count), file ());
  w_task.iface = io_client_type::POSIX;
#ifdef TIMERTB
  Timer t = Timer ();
  t.resumeTime ();
#endif
  auto write_tasks
      = task_m->build_write_task (w_task, static_cast<const char *> (buf));
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_write_task()," << std::fixed << std::setprecision (10)
          << t.pauseTime () << "\n";
  std::cout << stream1.str ();
#endif

  int index = 0;
  std::string write_data (static_cast<const char *> (buf));
  for (auto task : write_tasks)
    {
      if (task->addDataspace)
        {
          if (write_data.length () >= task->source.offset + task->source.size)
            {
              auto data
                  = write_data.substr (task->source.offset, task->source.size);
              data_m->put (DATASPACE_DB, task->destination.filename, data,
                           std::to_string (task->destination.server));
            }
          else
            {
              data_m->put (DATASPACE_DB, task->destination.filename,
                           write_data,
                           std::to_string (task->destination.server));
            }
        }
      if (task->publish)
        {
          if (count < task->source.size)
            mdm->update_write_task_info (*task, filename, count);
          else
            mdm->update_write_task_info (*task, filename, task->source.size);
          client_queue->publish_task (task);
        }
      else
        {
          mdm->update_write_task_info (*task, filename, task->source.size);
        }
      index++;
    }
  return write_tasks;
}

size_t
dtio::posix::write_wait (std::vector<task *> tasks)
{
  size_t total_size_written = 0;
  auto map_client = dtio_system::getInstance (LIB)->map_client ();
  auto map_server = dtio_system::getInstance (LIB)->map_server ();
  for (auto task : tasks)
    {
      int count = 0;
      Timer wait_timer = Timer ();

      while (!map_server->exists (table::WRITE_FINISHED_DB,
                                  task->destination.filename,
                                  std::to_string (-1)))
        {
          wait_timer.pauseTime ();
          if (wait_timer.getElapsedTime () > 5 && count % 10000 == 0)
            {
              int rank;
              MPI_Comm_rank (MPI_COMM_WORLD, &rank);
              std::stringstream stream;
              stream << "Waiting for task more than 5 seconds rank:" << rank
                     << "dataspace_id:" << task->destination.filename;
              count++;
            }
          wait_timer.resumeTime ();
        }
      map_server->remove (table::WRITE_FINISHED_DB, task->destination.filename,
                          std::to_string (-1));
      map_client->remove (table::DATASPACE_DB, task->destination.filename,
                          std::to_string (task->destination.server));
      total_size_written += task->destination.size;
      delete (task);
    }
  return total_size_written;
}

ssize_t
dtio::posix::write (int fd, const void *buf, size_t count)
{
  DTIO_LOG_DEBUG ("[POSIX] Write Entered");
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto map_client = dtio_system::getInstance (LIB)->map_client ();
  auto map_server = dtio_system::getInstance (LIB)->map_server ();
  auto task_m = task_builder::getInstance (LIB);
  auto data_m = data_manager::getInstance (LIB);
  auto filename = mdm->get_filename (fd);
  auto offset = mdm->get_fp (filename);
  if (!mdm->is_opened (filename))
    throw std::runtime_error ("dtio::write() file not opened!");
  auto w_task
      = task (task_type::WRITE_TASK, file (filename, offset, count), file ());

  w_task.iface = io_client_type::POSIX;
#ifdef TIMERTB
  Timer t = Timer ();
  t.resumeTime ();
#endif
  auto write_tasks
      = task_m->build_write_task (w_task, static_cast<const char *> (buf));
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_write_task()," << std::fixed << std::setprecision (10)
          << t.pauseTime () << "\n";
  std::cout << stream1.str ();
#endif

  int index = 0;
  std::string write_data (static_cast<const char *> (buf));
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
              data_m->put (DATASPACE_DB, tsk->source.filename, data,
                           std::to_string (tsk->destination.server));
            }
          else
            {
              data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
                           std::to_string (tsk->destination.server));
            }
        }
      if (tsk->publish)
        {
          if (count < tsk->source.size)
            mdm->update_write_task_info (*tsk, filename, count);
          else
            mdm->update_write_task_info (*tsk, filename, tsk->source.size);
          client_queue->publish_task (tsk);
          task_ids.emplace_back (std::make_pair (
              tsk->source.filename, std::to_string (tsk->destination.server)));
        }
      else
        {
          mdm->update_write_task_info (*tsk, filename, tsk->source.size);
        }

      index++;
      delete tsk;
    }
  for (auto task_id : task_ids)
    {
      while (!map_server->exists (table::WRITE_FINISHED_DB, task_id.first,
                                  std::to_string (-1)))
        {
        }
      map_server->remove (table::WRITE_FINISHED_DB, task_id.first,
                          std::to_string (-1));
      map_client->remove (table::DATASPACE_DB, task_id.first, task_id.second);
    }
  return count;
}

int
dtio::posix::__fxstat64 (int __ver, int fd, struct stat64 *buf)
{ // TODO: implement
  return 0;
}

int
dtio::posix::__open_2 (const char *path, int oflag)
{
  return open(path, oflag);
}

int
dtio::posix::__fxstat (int __ver, int fd, struct stat *buf)
{ // TODO: implement
  return 0;
}

int
dtio::posix::__fxstatat (int __ver, int __fildes, const char *__filename,
                         struct stat *__stat_buf, int __flag)
{ // TODO: implement
  return 0;
}

int
dtio::posix::__xstat (int __ver, const char *__filename,
                      struct stat *__stat_buf)
{ // TODO: implement
  return 0;
}

int
dtio::posix::__xstat64 (int __ver, const char *__filename,
                        struct stat64 *__stat_buf)
{ // TODO: implement
  return 0;
}

int
dtio::posix::__lxstat (int __ver, const char *__filename,
                       struct stat *__stat_buf)
{ // TODO: implement
  return 0;
}

int
dtio::posix::__fxstatat64 (int __ver, int __fildes, const char *__filename,
                           struct stat64 *__stat_buf, int __flag)
{ // TODO: implement
  return 0;
}

int
dtio::posix::__lxstat64 (int __ver, const char *__filename,
                         struct stat64 *__stat_buf)
{ // TODO: implement
  return 0;
}
