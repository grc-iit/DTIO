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
#include <dtiomod/dtiomod_client.h> // Make sure this actually gets included
#include <fcntl.h>
#include <iomanip>
#include <string>
#include <sys/stat.h>
#include <zconf.h>
// #include <adapter/posix/posix_api.h>

// #include <filesystem>

// namespace stdfs = std::filesystem;
// std::shared_ptr<dtio::posix::PosixApi> dtio::posix::PosixApi::instance =
// nullptr;

int temp_fd = -1;

int
dtio::posix::open (const char *filename, int flags)
{
  printf ("dtio posix open\n");
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  bool publish_task = false;

  if (!mdm->is_created (filename))
    {
      if (!(flags & O_CREAT))
        {
          int existing_size = 0;
          if (ConfigManager::get_instance ()->CHECKFS)
            {
              struct stat st;
              if (stat ((strncmp (filename, "dtio://", 7) == 0)
                            ? (filename + 7)
                            : filename,
                        &st)
                  == 0)
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
              if (mdm->update_on_open (filename, flags, 0, &fd, existing_size)
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
          if (mdm->create (filename, flags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
          publish_task = true;
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
          publish_task = true;
        }
      else
        return -1;
    }
  return fd;
}

int
dtio::posix::open (const char *filename, int flags, mode_t mode)
{
  printf ("dtio posix open 2\n");
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  bool publish_task = false;
  if (!mdm->is_created (filename))
    {
      if (!(flags & O_CREAT))
        {
          int existing_size = 0;
          if (ConfigManager::get_instance ()->CHECKFS)
            {
              struct stat st;
              if (stat ((strncmp (filename, "dtio://", 7) == 0)
                            ? (filename + 7)
                            : filename,
                        &st)
                  == 0)
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
              if (mdm->update_on_open (filename, flags, mode, &fd,
                                       existing_size)
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
          if (mdm->create (filename, flags, mode, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
          publish_task = true;
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
          publish_task = true;
        }
      else
        return -1;
    }
  return fd;
}

int
dtio::posix::open64 (const char *filename, int flags)
{
  printf ("dtio posix open64\n");
  DTIO_LOG_DEBUG ("[POSIX] Open64 filename " << filename);
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  bool publish_task = false;
  if (!mdm->is_created (filename))
    {
      if (!(flags & O_CREAT))
        {
          DTIO_LOG_TRACE ("[POSIX] Open64 file not marked created");
          int existing_size = 0;
          if (ConfigManager::get_instance ()->CHECKFS)
            {
              struct stat st;
              if (stat ((strncmp (filename, "dtio://", 7) == 0)
                            ? (filename + 7)
                            : filename,
                        &st)
                  == 0)
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
              if (mdm->update_on_open (filename, flags, 0, &fd, existing_size)
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
          if (mdm->create (filename, flags, 0, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
          publish_task = true;
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
          publish_task = true;
        }
      else
        {
          DTIO_LOG_DEBUG ("[POSIX] Open64 file already opened");
          return -1;
        }
    }
  // TODO switch with prefetching task
  // if (publish_task && ConfigManager::get_instance()->WORKER_STAGING_SIZE !=
  // 0) {
  //   // Publish staging task
  //   auto f = file (std::string (filename), 0, 0);
  //   auto s_task = task (task_type::STAGING_TASK, f);
  //   auto client_queue
  //     = dtio_system::getInstance (LIB)->get_client_queue
  //     (CLIENT_TASK_SUBJECT);
  //   // client_queue->publish_task (&s_task);
  // }
  return fd;
}

int
dtio::posix::open64 (const char *filename, int flags, mode_t mode)
{
  printf ("dtio posix open64 2\n");
  DTIO_LOG_DEBUG ("[POSIX] Open64 filename " << filename);
  auto mdm = metadata_manager::getInstance (LIB);
  int fd;
  bool file_exists_in_fs = false;
  bool publish_task = false;
  if (!mdm->is_created (filename))
    {
      if (!(flags & O_CREAT))
        {
          DTIO_LOG_TRACE ("[POSIX] Open64 file not marked created");
          int existing_size = 0;
          if (ConfigManager::get_instance ()->CHECKFS)
            {
              struct stat st;
              if (stat ((strncmp (filename, "dtio://", 7) == 0)
                            ? (filename + 7)
                            : filename,
                        &st)
                  == 0)
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
              if (mdm->update_on_open (filename, flags, mode, &fd,
                                       existing_size)
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
          if (mdm->create (filename, flags, mode, &fd) != SUCCESS)
            {
              throw std::runtime_error ("dtio::posix::open() create failed!");
            }
          publish_task = true;
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
          publish_task = true;
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
  auto map_client = dtio_system::getInstance (LIB)->map_client ();
  auto map_server = dtio_system::getInstance (LIB)->map_server ();
  auto task_m = dtio_system::getInstance (LIB)->task_composer ();
  auto data_m = data_manager::getInstance (LIB);
  file_stat st = mdm->get_stat (pathname);
  auto offset = st.file_pointer;
  if (!mdm->is_created (pathname))
    {
      throw std::runtime_error ("dtio::posix::unlink() file doesn't exist!");
    }
  auto f = file (std::string (pathname), offset, 0);
  auto d_task = task (task_type::DELETE_TASK, f);
  if (ConfigManager::get_instance ()->USE_URING)
    {
      d_task.iface = io_client_type::URING;
    }
  else
    {
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
  file_stat st = mdm->get_stat (pathname);
  statbuf->st_size = st.file_size;
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
  auto pathname = mdm->get_filename (fd);
  if (!mdm->is_created (pathname))
    {
      return -1;
    }
  file_stat st = mdm->get_stat (pathname);
  statbuf->st_size = st.file_size;
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
  auto pathname = mdm->get_filename (fd);
  if (!mdm->is_created (pathname))
    {
      return -1;
    }
  file_stat st = mdm->get_stat (pathname);
  statbuf->st_size = st.file_size;
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
  file_stat st = mdm->get_stat (pathname);
  statbuf->st_size = st.file_size;
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
  if (!mdm->is_opened (fd))
    {
      return EBADF;
    }
  auto filename = mdm->get_filename (fd);
  file_stat st = mdm->get_stat (filename);
  if (st.flags & O_APPEND)
    return 0;
  auto size = st.file_size;
  auto fp = st.file_pointer;
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
  if (!mdm->is_opened (fd))
    {
      return EBADF;
    }
  auto filename = mdm->get_filename (fd);
  file_stat st = mdm->get_stat (filename);
  if (st.flags & O_APPEND)
    return 0;

  DTIO_LOG_DEBUG ("[POSIX] Seek filename ", filename);
  auto size = st.file_size;
  DTIO_LOG_DEBUG ("[POSIX] Seek filesize ", size);
  auto fp = st.file_pointer;
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

ssize_t
dtio::posix::read (int fd, void *buf, size_t count)
{
  auto mdm = metadata_manager::getInstance (LIB);
  auto task_m = dtio_system::getInstance (LIB)->task_composer ();
  auto data_m = data_manager::getInstance (LIB);

  auto filename = mdm->get_filename (fd);
  file_stat st = mdm->get_stat (filename);
  auto offset = st.file_pointer;
  bool check_fs = false;
  task *task_i;
  task_i = nullptr;

  CHIMAERA_CLIENT_INIT ();
  chi::dtiomod::Client client;
  client.Create (
      HSHM_MCTX,
      chi::DomainQuery::GetDirectHash (chi::SubDomainId::kGlobalContainers, 0),
      chi::DomainQuery::GetGlobalBcast (), "ipc_test");

  size_t data_size
      = hshm::Unit<size_t>::Megabytes ((size_t)ceil ((double)count / 1024.0));
  size_t data_offset = offset;
  hipc::FullPtr<char> orig_data
      = CHI_CLIENT->AllocateBuffer (HSHM_MCTX, data_size);
  hipc::FullPtr<char> filename_fptr
      = CHI_CLIENT->AllocateBuffer (HSHM_MCTX, DTIO_FILENAME_MAX);
  char *filename_ptr = filename_fptr.ptr_;
  sprintf (filename_ptr, "%s", filename.c_str ());
  char *data_ptr = orig_data.ptr_;
  client.Read (HSHM_MCTX, orig_data.shm_, data_size, data_offset,
               filename_fptr.shm_, DTIO_FILENAME_MAX, io_client_type::POSIX);

  memcpy ((char *)buf, data_ptr, count);

  // if (!mdm->is_opened (filename))
  //   {
  //     if (!ConfigManager::get_instance ()->CHECKFS)
  //       {
  // 	  DTIO_LOG_DEBUG("File not opened");
  //         return 0;
  //       }
  //     if (!mdm->is_created (filename))
  //       {
  //         check_fs = true;
  //       }
  //   }
  // auto r_task
  //     = task (task_type::READ_TASK, file (filename, offset, count), file
  //     ());
  // DTIO_LOG_INFO("Numbers from read task " << r_task.source.size << " " <<
  // r_task.destination.size << " " << count); r_task.check_fs = check_fs; if
  // (ConfigManager::get_instance()->USE_CACHE) {
  //   r_task.source.location = BUFFERS;
  // }
  // else {
  //   r_task.source.location = PFS;
  // }
  // if (ConfigManager::get_instance()->USE_URING) {
  //   r_task.iface = io_client_type::URING;
  // }
  // else {
  //   r_task.iface = io_client_type::POSIX; //io_client_type::POSIX;
  // }

  // auto tasks = task_m->build_read_task (r_task);

  return count;
}

ssize_t
dtio::posix::write (int fd, const void *buf, size_t count)
{
  DTIO_LOG_DEBUG ("[POSIX] Write Entered");
  printf ("DTIO Posix Write entered\n");
  auto mdm = metadata_manager::getInstance (LIB);
  printf ("Mdm\n");
  auto task_m = dtio_system::getInstance (LIB)->task_composer ();
  printf ("task comp\n");
  auto data_m = data_manager::getInstance (LIB);
  printf ("data manager\n");
  auto filename = mdm->get_filename (fd);
  printf ("filename\n");
  if (!mdm->is_opened (filename))
    throw std::runtime_error ("dtio::write() file not opened!");
  printf ("get stat\n");
  file_stat st = mdm->get_stat (filename);
  printf ("fp\n");
  auto offset = st.file_pointer;
  auto source = file ();
  source.offset = 0; // buffer offset, NOT file offset
  source.size = count;
  printf ("dest\n");
  auto destination = file (filename, offset, count);
  printf ("Doing the write now\n");
  CHIMAERA_CLIENT_INIT ();
  chi::dtiomod::Client client;
  client.Create (
      HSHM_MCTX,
      chi::DomainQuery::GetDirectHash (chi::SubDomainId::kGlobalContainers, 0),
      chi::DomainQuery::GetGlobalBcast (), "ipc_test");

  size_t data_size
      = hshm::Unit<size_t>::Megabytes ((size_t)ceil ((double)count / 1024.0));
  size_t data_offset = offset;
  hipc::FullPtr<char> orig_data
      = CHI_CLIENT->AllocateBuffer (HSHM_MCTX, data_size);
  hipc::FullPtr<char> filename_fptr
      = CHI_CLIENT->AllocateBuffer (HSHM_MCTX, DTIO_FILENAME_MAX);
  char *filename_ptr = filename_fptr.ptr_;
  sprintf (filename_ptr, "%s", filename.c_str ());
  char *data_ptr = orig_data.ptr_;
  memcpy (data_ptr, (const char *)buf, count);

  client.Write (HSHM_MCTX, orig_data.shm_, data_size, data_offset,
                filename_fptr.shm_, DTIO_FILENAME_MAX, io_client_type::POSIX);

  // auto w_task
  //     = task (task_type::WRITE_TASK, source, destination);

  // if (ConfigManager::get_instance()->USE_URING) {
  //   w_task.iface = io_client_type::URING;
  // }
  // else {
  //   w_task.iface = io_client_type::POSIX; // io_client_type::POSIX;
  // }
  // auto write_tasks
  //   = task_m->build_write_task (w_task, static_cast<const char *> (buf));

  return count;
}

int
dtio::posix::__fxstat64 (int __ver, int fd, struct stat64 *buf)
{ // TODO: implement
  myfstat64 (fd, buf);
  return 0;
}

int
dtio::posix::__open_2 (const char *path, int oflag)
{
  return open (path, oflag);
}

int
dtio::posix::__fxstat (int __ver, int fd, struct stat *buf)
{ // TODO: implement
  myfstat (fd, buf);
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
