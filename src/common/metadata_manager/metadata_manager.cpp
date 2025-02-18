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
#include "dtio/common/enumerations.h"
#include "dtio/common/logger.h"
#include <cstring>
#include <dtio/common/metadata_manager/metadata_manager.h>
#include <dtio/common/return_codes.h>
#include <dtio/common/utilities.h>
#include <hcl/common/debug.h>
#include <mpi.h>
#include <random>

std::shared_ptr<metadata_manager> metadata_manager::instance = nullptr;
/******************************************************************************
 *Interface
 ******************************************************************************/
bool
metadata_manager::is_created (std::string filename)
{
  auto iter = file_map.find (filename);
  return iter != file_map.end ();
}

int
metadata_manager::create (std::string filename, std::string mode, FILE *&fh)
{
  if (filename.length () > FILENAME_MAX)
    return MDM__FILENAME_MAX_LENGTH;
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  fh = fmemopen (nullptr, 1, mode.c_str ());
  file_stat stat = { fh, -1, 0, 0, 0, 0, mode, true };
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    file_map.erase (iter);
  fh_map.emplace (fh, filename);
  file_map.emplace (filename, stat);
  map->put (table::FILE_DB, filename, &stat, std::to_string (-1));
  /**
   * TODO: put in map for outstanding operations-> on fclose create a file
   * in the destination and flush buffer contents
   **/
  return SUCCESS;
}

int
metadata_manager::create (std::string filename, int flags, mode_t mode,
                          int *fd)
{
  if (filename.length () > FILENAME_MAX)
    return MDM__FILENAME_MAX_LENGTH;
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");

  *fd = random (); // random number, maybe use an HCL sequencer
  file_stat stat = { nullptr, *fd, 0, 0, flags, mode, "", true };
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    file_map.erase (iter);
  fd_map.emplace (*fd, filename);
  file_map.emplace (filename, stat);
  map->put (table::FILE_DB, filename, &stat, std::to_string (-1));
  /**
   * TODO: put in map for outstanding operations-> on fclose create a file
   * in the destination and flush buffer contents
   **/
  return SUCCESS;
}

bool
metadata_manager::is_opened (std::string filename)
{
  auto iter = file_map.find (filename);
  return iter != file_map.end () && iter->second.is_open;
}

int
metadata_manager::update_on_open (std::string filename, std::string mode,
                                  FILE *&fh)
{
#ifdef TIMERMDM
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  if (filename.length () > FILENAME_MAX)
    return MDM__FILENAME_MAX_LENGTH;
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  auto iter = file_map.find (filename);
  file_stat stat;

  fh = fmemopen (nullptr, 1, mode.c_str ());
  stat.file_size = iter->second.file_size;
  stat.fh = fh;
  stat.is_open = true;
  if (mode == "r" || mode == "r+")
    {
      stat.file_pointer = 0;
    }
  else if (mode == "w" || mode == "w+")
    {
      stat.file_pointer = 0;
      stat.file_size = 0;
      // remove_chunks (filename);
    }
  else if (mode == "a" || mode == "a+")
    {
      stat.file_pointer = stat.file_size;
    }

  iter = file_map.find (filename);
  if (iter != file_map.end ())
    file_map.erase (iter);
  fh_map.emplace (fh, filename);
  file_map.emplace (filename, stat);
  map->put (table::FILE_DB, filename, &stat, std::to_string (-1));
#ifdef TIMERMDM
  DTIO_LOG_TRACE("metadata_manager::update_on_open()," << t.pauseTime () << "\n");
#endif
  return SUCCESS;
}

int
metadata_manager::update_on_open (std::string filename, int flags, mode_t mode,
                                  int *fd, int existing_size)
{
#ifdef TIMERMDM
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  if (filename.length () > FILENAME_MAX)
    return MDM__FILENAME_MAX_LENGTH;
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  auto iter = file_map.find (filename);
  file_stat stat;

  *fd = random ();
  if (iter != file_map.end()) {
    stat.file_size = iter->second.file_size;
  }
  else {
    stat.file_size = existing_size;
  }
  stat.fd = *fd;
  stat.is_open = true;
  if (flags & O_RDONLY)
    {
      stat.file_pointer = 0;
    }
  else if (flags & O_WRONLY)
    {
      stat.file_pointer = 0;
      // stat.file_size = 0;
      // remove_chunks (filename);
    }
  else if (flags & O_RDWR)
    {
      stat.file_pointer = 0;
    }
  if (flags & O_TRUNC)
    {
      stat.file_size = 0;
    }
  if (flags & O_APPEND)
    {
      stat.file_pointer = stat.file_size;
    }
  
  iter = file_map.find (filename);
  if (iter != file_map.end ()) {
    file_map.erase (iter);
    DTIO_LOG_TRACE ("[DTIO][MDMAN][UPDATE_ON_OPEN] " << iter->second.file_size << " " << stat.file_size);
  }
  fd_map.emplace (*fd, filename);
  file_map.emplace (filename, stat);
  map->put (table::FILE_DB, filename, &stat, std::to_string (-1));
#ifdef TIMERMDM
  DTIO_LOG_TRACE("metadata_manager::update_on_open()," << t.pauseTime () << "\n");
#endif
  return SUCCESS;
}

bool
metadata_manager::is_opened (FILE *fh)
{
  auto iter1 = fh_map.find (fh);
  if (iter1 != fh_map.end ())
    {
      auto iter = file_map.find (iter1->second);
      return iter != file_map.end () && iter->second.is_open;
    }
  return false;
}

bool
metadata_manager::is_opened (int fd)
{
  auto iter1 = fd_map.find (fd);
  if (iter1 != fd_map.end ())
    {
      auto iter = file_map.find (iter1->second);
      return iter != file_map.end () && iter->second.is_open;
    }
  return false;
}

int
metadata_manager::remove_chunks (std::string &filename)
{
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+filemeta");
  std::string chunks_str
    = map->remove (table::FILE_CHUNK_DB, filename, std::to_string (-1));
  // std::vector<std::string> chunks = string_split (chunks_str);
  // for (const auto &chunk : chunks)
  //   {
  //     map->remove (table::CHUNK_DB, chunk, std::to_string (-1));
  //   }
  return SUCCESS;
}

int
metadata_manager::update_on_close (FILE *&fh)
{
  auto iter = fh_map.find (fh);
  if (iter != fh_map.end ())
    {
      auto iter2 = file_map.find (iter->second);
      if (iter2 != file_map.end ())
        {
          iter2->second.is_open = false;
        }
      fh_map.erase (iter);
      std::fclose (fh);
    }
  return SUCCESS;
}

int
metadata_manager::update_on_close (int fd)
{
  auto iter = fd_map.find (fd);
  if (iter != fd_map.end ())
    {
      auto iter2 = file_map.find (iter->second);
      if (iter2 != file_map.end ())
        {
          iter2->second.is_open = false;
        }
      fd_map.erase (iter);
    }
  return SUCCESS;
}

std::string
metadata_manager::get_filename (FILE *fh)
{
  auto iter = fh_map.find (fh);
  if (iter != fh_map.end ())
    return iter->second;
  return nullptr;
}

std::string
metadata_manager::get_filename (int fd)
{
  auto iter = fd_map.find (fd);
  if (iter != fd_map.end ())
    return iter->second;
  return nullptr;
}

std::size_t
metadata_manager::get_filesize (std::string filename)
{
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    {
      DTIO_LOG_TRACE ("[DTIO][MDMAN][GET_FILESIZE] " << filename << " "
                                                    << iter->second.file_size);
      return iter->second.file_size;
    }
  DTIO_LOG_TRACE ("[DTIO][MDMAN][GET_FILESIZE] " << filename << " FAILED");
  return 0;
}

std::string
metadata_manager::get_mode (std::string filename)
{
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    return iter->second.mode;
  return nullptr;
}

int
metadata_manager::get_flags (std::string filename)
{
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    return iter->second.flags;
  return 0;
}

mode_t
metadata_manager::get_posix_mode (std::string filename)
{
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    return iter->second.posix_mode;
  return 0;
}

long long int
metadata_manager::get_fp (const std::string &filename)
{
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    return static_cast<long long int> (iter->second.file_pointer);
  else
    return -1;
}

int
metadata_manager::update_read_task_info (std::vector<task> task_ks,
                                         std::string filename)
{
#ifdef TIMERMDM
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  file_stat fs;
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    fs = iter->second;
  for (int i = 0; i < task_ks.size (); ++i)
    {
      auto task_k = task_ks[i];
      assert (task_k.t_type == task_type::READ_TASK);
      update_on_read (filename, task_k.source.size);
    }
  map->put (table::FILE_DB, filename, &fs, std::to_string (-1));
#ifdef TIMERMDM
  DTIO_LOG_TRACE("metadata_manager::update_read_task_info()," << t.pauseTime ()
		 << "\n");
#endif
  return 0;
}

int
metadata_manager::update_write_task_info (std::vector<task> task_ks,
                                          std::string filename)
{
  auto filemap = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  // auto cmmap = dtio_system::getInstance (service_i)->map_client ("metadata+chunkmeta");
  auto fmmap = dtio_system::getInstance (service_i)->map_client ("metadata+filemeta");
  file_stat fs;
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    fs = iter->second;
  file_meta fm;
  for (int i = 0; i < task_ks.size (); ++i)
    {
      auto task_k = task_ks[i];
      update_on_write (task_k.destination.filename, task_k.destination.size,
                       task_k.destination.offset);
      // if (!task_k.meta_updated)
      //   {
      auto base_offset
	= (task_k.destination.offset / MAX_IO_UNIT) * MAX_IO_UNIT;
      chunk_meta cm;
      cm.actual_user_chunk = task_k.destination;
      cm.destination = task_k.destination;
      // cmmap->put (table::CHUNK_DB, filename + std::to_string (base_offset),
      //           &cm, std::to_string (-1));
      // std::cout << "Updating filemeta for file " << filename << std::endl;
      fmmap->get(table::FILE_CHUNK_DB, filename, std::to_string (-1), &fm);
      fm.append(&cm);
      fmmap->put (table::FILE_CHUNK_DB, filename, &fm, std::to_string (-1));
        // }
    }
  filemap->put (table::FILE_DB, filename, &fs, std::to_string (-1));
  return 0;
}

int
metadata_manager::update_on_seek (std::string filename, size_t offset,
                                  size_t origin)
{
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    {
      switch (origin)
        {
        case SEEK_SET:
          {
            // offset <= iter->second.file_size &&
            if (offset >= 0)
              {
                iter->second.file_pointer = offset;
              }
            break;
          }
        case SEEK_CUR:
          {
            // if (iter->second.file_pointer + offset <=
            // iter->second.file_size) {
            iter->second.file_pointer += offset;
            // }
            break;
          }
        case SEEK_END:
          {
            // if (offset <= iter->second.file_size) {
            iter->second.file_pointer = iter->second.file_size - offset;
            // }
            break;
          }
        default:
          std::cerr << "fseek origin error\n";
          return MDM__UPDATE_ON_FSEEK_FAILED;
        }
      map->put (table::FILE_DB, filename, &iter->second, std::to_string (-1));
    }
  return SUCCESS;
}

void
metadata_manager::update_on_read (std::string filename, size_t size)
{
  std::shared_ptr<distributed_hashmap> map
      = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    {
      file_stat fs = iter->second;
      fs.file_pointer += size;
      file_map[filename] = fs;
      map->put (table::FILE_DB, filename, &fs, std::to_string (-1));
    }
}

void
metadata_manager::update_on_write (std::string filename, size_t size,
                                   size_t offset)
{
  DTIO_LOG_TRACE("Update on write" << std::endl);
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    {
      file_stat fs = iter->second;
      if (fs.file_size < offset + size)
        {
          fs.file_size = offset + size;
        }
      fs.file_pointer = offset + size;
      file_map[filename] = fs;
      map->put (table::FILE_DB, filename, &fs, std::to_string (-1));
    }
  else {
    DTIO_LOG_ERROR("Update on write file not found " << filename);
  }
  DTIO_LOG_TRACE("[DTIO][UPDATE_ON_WRITE] " << filename << " " << offset << " " << size << " ");
}

void
metadata_manager::update_on_delete (std::string filename)
{
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    {
      file_map.erase (filename);
      file_stat fs = iter->second;
      map->remove (table::FILE_DB, filename, std::to_string (-1));
    }
}

std::vector<chunk_meta>
metadata_manager::fetch_chunks (task task)
{
#ifdef TIMERMDM
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif

  assert (task.t_type == task_type::READ_TASK);
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+chunkmeta");

  auto remaining_data = task.source.size;
  auto chunks = std::vector<chunk_meta> ();
  chunk_meta cm;
  size_t source_offset = task.source.offset;
  while (remaining_data > 0)
    {
      auto base_offset = (source_offset / MAX_IO_UNIT) * MAX_IO_UNIT;
      bool chunk_avail = map->get (
          table::CHUNK_DB, task.source.filename + std::to_string (base_offset),
          std::to_string (-1), &cm);
      size_t size_to_read;
      if (chunk_avail)
        {
          size_t bucket_offset = source_offset - base_offset;
          cm.destination.offset = bucket_offset;

          if (bucket_offset + remaining_data <= MAX_IO_UNIT)
            {
              size_to_read = remaining_data;
            }
          else
            {
              size_to_read = MAX_IO_UNIT - bucket_offset;
            }
          cm.destination.size = size_to_read;
        }
      else
        {
          cm.actual_user_chunk.offset = source_offset;
          cm.actual_user_chunk.size = remaining_data;
          strncpy (cm.actual_user_chunk.filename, task.source.filename,
                   DTIO_FILENAME_MAX);
          cm.actual_user_chunk.location = location_type::PFS;
          cm.destination.location = location_type::PFS;
          cm.destination.size = cm.actual_user_chunk.size;
          strncpy (cm.destination.filename, cm.actual_user_chunk.filename,
                   DTIO_FILENAME_MAX);
          cm.destination.offset = cm.actual_user_chunk.offset;
          cm.destination.worker = -1;
          size_to_read = cm.actual_user_chunk.size;
        }
      chunks.push_back (cm);
      if (remaining_data >= size_to_read)
        remaining_data -= size_to_read;
      else
        remaining_data = 0;
      source_offset += size_to_read;
    }
#ifdef TIMERMDM
  DTIO_LOG_INFO("metadata_manager::fetch_chunks()," << t.pauseTime () << "\n");
#endif
  return chunks;
}

int
metadata_manager::update_delete_task_info (task task_k, std::string filename)
{
#ifdef TIMERMDM
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  auto map = dtio_system::getInstance (service_i)->map_client ("metadata+fs");
  file_stat fs;
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    fs = iter->second;
  update_on_delete (task_k.source.filename);
  // if (!task_k.meta_updated) {
  //   auto chunk_index = (task_k.source.offset / MAX_IO_UNIT);
  //   auto base_offset = (task_k.source.offset / MAX_IO_UNIT) * MAX_IO_UNIT;
  //   chunk_meta cm;
  //   cm.actual_user_chunk = task_k.source;
  //   cm.destination = task_k.destination;
  //   map->remove(table::CHUNK_DB, filename + std::to_string(base_offset),
  //   &cm);
  // }
  map->remove (table::FILE_DB, filename, std::to_string (-1));
#ifdef TIMERMDM
  DTIO_LOG_INFO("metadata_manager::update_delete_task_info()," << t.pauseTime () << "\n");
#endif
  return 0;
}

int
metadata_manager::update_write_task_info (task task_k, std::string filename,
                                          std::size_t io_size)
{
  auto fmmap = dtio_system::getInstance (service_i)->map_client ("metadata+filemeta");
#ifdef TIMERMDM
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  // auto map = dtio_system::getInstance (service_i)->map_client ();
  file_stat fs;
  auto iter = file_map.find (filename);
  if (iter != file_map.end ())
    fs = iter->second;
  update_on_write (filename, io_size, task_k.destination.offset);
      // if (!task_k.meta_updated)
      //   {
  auto base_offset
    = (task_k.destination.offset / MAX_IO_UNIT) * MAX_IO_UNIT;
  chunk_meta cm;
  cm.actual_user_chunk = task_k.destination;
  cm.destination = task_k.destination;
  // cmmap->put (table::CHUNK_DB, filename + std::to_string (base_offset),
  //           &cm, std::to_string (-1));
  // std::cout << "Updating filemeta for file " << filename << std::endl;
  file_meta fm;
  fmmap->get(table::FILE_CHUNK_DB, filename, std::to_string (-1), &fm);
  fm.append(&cm);
  // std::cout << fm.chunks[fm.current_chunk_index - 1].actual_user_chunk.size << std::endl;
  fmmap->put (table::FILE_CHUNK_DB, filename, &fm, std::to_string (-1));
  // std::cout << fm.chunks[fm.current_chunk_index - 1].actual_user_chunk.size << std::endl;
  // }

  // if (!task_k.meta_updated)
  //   {
  //     auto chunk_index = (task_k.source.offset / MAX_IO_UNIT);
  //     auto base_offset = (task_k.source.offset / MAX_IO_UNIT) * MAX_IO_UNIT;
  //     chunk_meta cm;
  //     cm.actual_user_chunk = task_k.source;
  //     cm.destination = task_k.source;
  //     map->put (table::CHUNK_DB, filename + std::to_string (base_offset),
  //               &cm, std::to_string (-1));
  //   }
  // map->put (table::FILE_DB, filename, &fs, std::to_string (-1));
#ifdef TIMERMDM
  DTIO_LOG_INFO("metadata_manager::update_write_task_info()," << t.pauseTime () << "\n");
#endif
  return 0;
}
