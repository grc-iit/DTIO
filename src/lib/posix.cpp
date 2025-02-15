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
#include <dtio/drivers/posix.h>
#include <fcntl.h>
#include <hcl/common/debug.h>
#include <iomanip>
#include <sys/stat.h>
#include <zconf.h>
#include <string>
// #include <adapter/posix/posix_api.h>

// #include <filesystem>

// namespace stdfs = std::filesystem;
// std::shared_ptr<dtio::posix::PosixApi> dtio::posix::PosixApi::instance =
// nullptr;

int temp_fd = -1;

int dtio_worker_write(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  std::shared_ptr<distributed_hashmap> map_client =
      dtio_system::getInstance(LIB)->map_client();
  std::shared_ptr<distributed_hashmap> map_server =
      dtio_system::getInstance(LIB)->map_server();
  auto source = tsk[task_idx]->source;
  size_t chunk_index = (source.offset / MAX_IO_UNIT);
  size_t base_offset = chunk_index * MAX_IO_UNIT + source.offset % MAX_IO_UNIT;
  chunk_meta chunk_meta1;
  bool chunk_avail = !tsk[task_idx]->addDataspace;
  // map_client->get(
  //     table::CHUNK_DB,
  //     tsk[task_idx]->source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
  //     std::to_string(-1), &chunk_meta1);

#ifdef TIMERDM
  hcl::Timer t0 = hcl::Timer();
  t0.resumeTime();
#endif
  // FIXME: modify the data to pull from local buffers if there is no dataspace (task is sync)
#if(STACK_ALLOCATION)
  {
    // char *data;
    // data = (char *)malloc(MAX_IO_UNIT);
    char data[MAX_IO_UNIT];
    if (!chunk_avail) {
      map_client->get(DATASPACE_DB, tsk[task_idx]->destination.filename,
		      std::to_string(tsk[task_idx]->destination.server), data);
    }
    else {
      // It's a chunk, this should be the filename with chunk id
      map_client->get(DATASPACE_DB, tsk[task_idx]->source.filename,
		      std::to_string(tsk[task_idx]->destination.server), data);
    }
#ifdef TIMERDM
    std::stringstream stream;
    stream << "posix_client::write()::get_data," << std::fixed
	   << std::setprecision(10) << t0.pauseTime() << "\n";
    std::cout << stream.str();
#endif
    // std::string file_path;
    char *filepath = (strncmp(tsk[task_idx]->destination.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->destination.filename + 7) : tsk[task_idx]->destination.filename ;
    int fd;
    if (temp_fd == -1) {
      fd = open64(filepath, O_RDWR | O_CREAT, 0664); // "w+"
      temp_fd = fd;
    }
    else {
      fd = temp_fd;
    }
    if (fd < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }
    lseek64(fd, tsk[task_idx]->destination.offset, SEEK_SET);
    auto count = write(fd, data, tsk[task_idx]->destination.size);
    if (count != tsk[task_idx]->destination.size)
      std::cerr << "written less" << count << "\n";
    // free(data);
    // close(fd);
  }
#else
  {
    char *data = (char *)malloc(MAX_IO_UNIT);
    if (!chunk_avail) {
      map_client->get(DATASPACE_DB, tsk[task_idx]->destination.filename,
		      std::to_string(tsk[task_idx]->destination.server), data);
    }
    else {
      // It's a chunk, this should be the filename with chunk id
      map_client->get(DATASPACE_DB, tsk[task_idx]->source.filename,
		      std::to_string(tsk[task_idx]->destination.server), data);
    }
#ifdef TIMERDM
    std::stringstream stream;
    stream << "posix_client::write()::get_data," << std::fixed
	   << std::setprecision(10) << t0.pauseTime() << "\n";
    std::cout << stream.str();
#endif
    // std::string file_path;
    char *filepath = (strncmp(tsk[task_idx]->destination.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->destination.filename + 7) : tsk[task_idx]->destination.filename ;
    int fd;
    if (temp_fd == -1) {
      fd = open64(filepath, O_RDWR | O_CREAT, 0664); // "w+"
      temp_fd = fd;
    }
    else {
      fd = temp_fd;
    }
    if (fd < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }
    lseek64(fd, tsk[task_idx]->destination.offset, SEEK_SET);
    auto count = write(fd, data, tsk[task_idx]->destination.size);
    if (count != tsk[task_idx]->destination.size)
      std::cerr << "written less" << count << "\n";
    free(data);
    // close(fd);
  }
#endif
//   if (chunk_meta1.destination.location == location_type::CACHE) {
//     /*
//      * New I/O
//      */
//     auto file_id = static_cast<int64_t>(
//         std::chrono::duration_cast<std::chrono::microseconds>(
//             std::chrono::system_clock::now().time_since_epoch())
//             .count());
//     file_path = dir + std::to_string(file_id);
// #ifdef DEBUG
//     std::cout << strlen(data) << " chunk index:" << chunk_index
//               << " dataspaceId:" << tsk[task_idx]->destination.filename
//               << " created filename:" << file_path
//               << " clientId: " << tsk[task_idx]->destination.server << "\n";
// #endif
//     int fd = open64(file_path.c_str(), O_RDWR | O_CREAT); // "w+"
//     auto count = write(fd, data, tsk[task_idx]->destination.size);
//     if (count != tsk[task_idx]->destination.size)
//       std::cerr << "written less" << count << "\n";
//     close(fd);
//   } else {
//     /*cd
//      * existing I/O
//      */
// #ifdef DEBUG
//     std::cout << "update file  " << strlen(data)
//               << " chunk index:" << chunk_index
//               << " dataspaceId:" << tsk[task_idx]->destination.filename
//               << " clientId: " << tsk[task_idx]->destination.server << "\n";
// #endif

//     file_path = chunk_meta1.destination.filename;
//     int fd = open64(chunk_meta1.destination.filename, O_RDWR | O_CREAT); // "r+"
//     lseek64(fd, tsk[task_idx]->source.offset - base_offset, SEEK_SET);
//     write(fd, data, tsk[task_idx]->source.size);
//     close(fd);
//   }
// chunk_meta1.actual_user_chunk = tsk[task_idx]->source;
// chunk_meta1.destination.location = BUFFERS;
// strncpy(chunk_meta1.destination.filename, file_path.c_str(), DTIO_FILENAME_MAX);
// chunk_meta1.destination.offset = 0;
// chunk_meta1.destination.size = tsk[task_idx]->destination.size;
// chunk_meta1.destination.worker = worker_index;
// #ifdef TIMERMDM
//   hcl::Timer t1 = hcl::Timer();
//   t1.resumeTime();
// #endif
// map_client->put(table::CHUNK_DB,
//                 tsk[task_idx]->source.filename +
// 		  std::to_string(chunk_index * MAX_IO_UNIT),
//                 &chunk_meta1, std::to_string(-1));

//    map_client->remove(DATASPACE_DB,task.destination.filename,
//    std::to_string(task.destination.server));
// #ifdef TIMERMDM
//   std::cout << "posix_client::write()::update_meta," << t1.pauseTime() << "\n";
// #endif
  // NOTE Removing this makes I/O asynchronous by default
  map_server->put(table::WRITE_FINISHED_DB, std::to_string(tsk[task_idx]->task_id),
                  std::to_string(-1), std::to_string(-1));
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "posix_client::write()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  }
  return 0;
}

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
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance (LIB);
  auto offset = mdm->get_fp (pathname);
  if (!mdm->is_created (pathname))
    {
      throw std::runtime_error ("dtio::posix::unlink() file doesn't exist!");
    }
  auto f = file (std::string (pathname), offset, 0);
  auto d_task = task (task_type::DELETE_TASK, f);
  if (ConfigManager::get_instance()->USE_URING) {
    d_task.iface = io_client_type::URING;
  }
  else {
    d_task.iface = io_client_type::POSIX;
  }
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
  DTIO_LOG_TRACE ("[DTIO][POSIX][STAT] " << pathname << " "
                                        << statbuf->st_size);
  return 0;
}

int
dtio::posix::myfstat (int fd, struct stat *statbuf)
{
  // Note: I actually had to rename stat in order to use it for file existence
  // checks
  // FIXME right now we just grab file size, which is ok for IOR but also bad
  auto mdm = metadata_manager::getInstance (LIB);
  auto pathname = mdm->get_filename(fd);
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
dtio::posix::myfstat64 (int fd, struct stat64 *statbuf)
{
  // Note: I actually had to rename stat in order to use it for file existence
  // checks
  // FIXME right now we just grab file size, which is ok for IOR but also bad
  auto mdm = metadata_manager::getInstance (LIB);
  auto pathname = mdm->get_filename(fd);
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
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto filename = mdm->get_filename (fd);
  auto offset = mdm->get_fp (filename);
  if (!mdm->is_opened (filename))
    return std::vector<task> ();
  auto r_task
      = task (task_type::READ_TASK, file (filename, offset, count), file ());
  if (ConfigManager::get_instance()->USE_URING) {
    r_task.iface = io_client_type::URING;
  }
  else {
    r_task.iface = io_client_type::POSIX;
  }
#ifdef TIMERTB
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  auto tasks = task_m->build_read_task (r_task);
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_read_task()," << std::fixed << std::setprecision (10)
          << t.pauseTime () << "\n";
  DTIO_LOG_TRACE(stream1.str ());
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
            hcl::Timer wait_timer = hcl::Timer ();
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
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance (LIB);
  auto filename = mdm->get_filename (fd);
  auto offset = mdm->get_fp (filename);
  bool check_fs = false;
  task *task_i;
  task_i = nullptr;

  // bool pfs = true;
  // if (filename[27] == 'c') {
  //   std::cout << "Doing buffer read" << std::endl;
  //   pfs = false;
  // }
  // else {
  //   std::cout << "Doing PFS read" << std::endl;
  // }

  if (!mdm->is_opened (filename))
    {
      if (!ConfigManager::get_instance ()->CHECKFS)
        {
	  std::cout << "File not opened" << std::endl;
          return 0;
        }
      if (!mdm->is_created (filename))
        {
          check_fs = true;
        }
    }
  auto r_task
      = task (task_type::READ_TASK, file (filename, offset, count), file ());
  std::cout << "Numbers from read task " << r_task.source.size << " " << r_task.destination.size << " " << count << std::endl;
  r_task.check_fs = check_fs;
  // if (pfs) {
  //   r_task.source.location = PFS;
  // }
  // else {
  //   r_task.source.location = BUFFERS;
  // }
  if (ConfigManager::get_instance()->USE_CACHE) {
    r_task.source.location = BUFFERS;
  }
  if (ConfigManager::get_instance()->USE_URING) {
    r_task.iface = io_client_type::URING;
  }
  else {
    r_task.iface = io_client_type::POSIX; //io_client_type::POSIX;
  }
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
		       t.source.size);

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
		  std::cout << "Raw data get" << std::endl;

		  data_m->get (DATASPACE_DB, t.source.filename,
			       std::to_string (t.destination.server), char_data);

		  std::cout << char_data << std::endl;
		  std::cout << "Get data into " << ptr_pos << " from " << t.source.offset << " size " << t.source.size << std::endl;
		  strncpy ((char *)buf + ptr_pos,
			   char_data, t.source.size); //  + t.source.offset
		  std::cout << "Success" << std::endl;
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
	      // client_queue->publish_task (&t);
	      std::cout << "Waiting for " << t.source.filename << std::endl;
	      // while (!data_m->exists (DATASPACE_DB, t.source.filename,
	      // 			      std::to_string (t.destination.server)))
	      // 	{
	      // 	  // std::cerr<<"looping\n";
	      // 	}
	      std::cout << "Raw data get" << std::endl;

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
		    std::cout << "Range not resolved at " << range_bound[i] << std::endl;
		    range_resolved = false;
		    break;
		  }
		}
	      }

	      free(range_bound);
	      // Range resolved on current tasks lower down

	      // Resolve range on current task
	      std::cout << "Pulling from resolved range" << std::endl;
	      if (range_resolved) {
		for (unsigned i = resolve_dts.size(); i-- > 0; ) {
		  // Currently, we're just iterating backwards so newer DTs overwrite older ones.
		  // TODO better way to do this is to resolve the range by precalculating the offsets and sizes that get pulled into the buffer from each DT.
		  map_client->get(DATASPACE_DB, resolve_dts[i].filename, std::to_string(resolve_dts[i].server), (char *)buf + ptr_pos + resolve_dts[i].offset - 0,
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

	      std::cout << "Success" << std::endl;

	      // free(char_data);
	      // data_m->remove (DATASPACE_DB, t.source.filename,
	      // 		      std::to_string (t.destination.server));

	      size_read += t.destination.size;
	      break;
	    }
	  case CACHE:
	    {
	      DTIO_LOG_TRACE("Cache" << std::endl);
	      std::cout << "Cache" << std::endl;
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
  mdm->update_read_task_info (tasks, filename);
  return size_read;
}

std::vector<task *>
dtio::posix::write_async (int fd, const void *buf, size_t count)
{
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance (LIB);
  auto filename = mdm->get_filename (fd);
  auto offset = mdm->get_fp (filename);
  if (!mdm->is_opened (filename))
    throw std::runtime_error ("dtio::posix::write() file not opened!");
  auto w_task
      = task (task_type::WRITE_TASK, file (filename, offset, count), file ());
  if (ConfigManager::get_instance()->USE_URING) {
    w_task.iface = io_client_type::URING;
  }
  else {
    w_task.iface = io_client_type::POSIX;
  }
#ifdef TIMERTB
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  auto write_tasks
      = task_m->build_write_task (w_task, static_cast<const char *> (buf));
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_write_task()," << std::fixed << std::setprecision (10)
          << t.pauseTime () << "\n";
  DTIO_LOG_TRACE(stream1.str ());
#endif

  int index = 0;
  std::string write_data (static_cast<const char *> (buf), count);
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
      hcl::Timer wait_timer = hcl::Timer ();

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
  std::cout << "Entering write" << std::endl;
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
  DTIO_LOG_DEBUG ("[POSIX] Write Entered");
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto map_client = dtio_system::getInstance (LIB)->map_client ();
  auto map_server = dtio_system::getInstance (LIB)->map_server ();
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance (LIB);
  auto filename = mdm->get_filename (fd);
  auto offset = mdm->get_fp (filename);
  t.pauseTime ();
  std::cout << "Time initializing task information " << t.getElapsedTime() << std::endl;
  t.resumeTime();
  if (!mdm->is_opened (filename))
    throw std::runtime_error ("dtio::write() file not opened!");
  auto source = file();
  source.offset = 0; // buffer offset, NOT file offset
  source.size = count;
  auto destination = file(filename, offset, count);
  auto w_task
      = task (task_type::WRITE_TASK, source, destination);

  if (ConfigManager::get_instance()->USE_URING) {
    w_task.iface = io_client_type::URING;
  }
  else {
    w_task.iface = io_client_type::POSIX; // io_client_type::POSIX;
  }
// #ifdef TIMERTB
//   hcl::Timer t = hcl::Timer ();
//   t.resumeTime ();
// #endif
  auto write_tasks
    = task_m->build_write_task (w_task, static_cast<const char *> (buf));
// #ifdef TIMERTB
//   std::stringstream stream1;
//   stream1 << "build_write_task()," << std::fixed << std::setprecision (10)
//           << t.pauseTime () << "\n";
//   DTIO_LOG_TRACE(stream1.str ());
// #endif

  t.pauseTime();
  std::cout << "Time to construct task " << t.getElapsedTime() << std::endl;
  t.resumeTime();
  const char *write_data_char = static_cast<const char *>(buf);
  std::string write_data (static_cast<const char *> (buf), count);

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

	  // dtio_worker_write(&tsk);
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
  t.pauseTime();
  std::cout << "Time to publish and execute task " << t.getElapsedTime() << std::endl;
  t.resumeTime();

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
  t.pauseTime();
  std::cout << "Time from synchronicity " << t.getElapsedTime() << std::endl;

  return count;
}

int
dtio::posix::__fxstat64 (int __ver, int fd, struct stat64 *buf)
{ // TODO: implement
  myfstat64(fd, buf);
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
  myfstat(fd, buf);
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
