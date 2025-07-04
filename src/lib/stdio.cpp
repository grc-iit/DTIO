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

  CHIMAERA_CLIENT_INIT();
  chi::dtio::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  
  size_t data_size = hshm::Unit<size_t>::Megabytes((size_t)ceil((double)(size * count) / 1024.0));
  size_t data_offset = offset;
  hipc::FullPtr<char> orig_data =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, data_size);
  hipc::FullPtr<char> filename_fptr =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, DTIO_FILENAME_MAX);
  char *filename_ptr = filename.ptr_;
  sprintf(filename_ptr, filename);
  char *data_ptr = orig_data.ptr_;
  client.Read(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0), orig_data.shm_,
	       data_size, data_offset, filename_fptr.shm_, DTIO_FILENAME_MAX);

  memcpy((char *)buf, data_ptr, count);
  
  // if (!mdm->is_opened(filename)) {
  //   if (!ConfigManager::get_instance ()->CHECKFS)
  //     {
  // 	return 0;
  //     }
  //   if (!mdm->is_created (filename))
  //     {
  // 	check_fs = true;
  //     }
  // }
  // auto r_task = task(task_type::READ_TASK, file(filename, offset, size * count), file());
  // r_task.check_fs = check_fs;
  // r_task.special_type = special_type;
  // if (ConfigManager::get_instance()->USE_CACHE) {
  //   r_task.source.location = BUFFERS;
  // }
  // else {
  //   r_task.source.location = PFS;
  // }

  // r_task.iface = io_client_type::STDIO;
  // auto tasks = task_m->build_read_task(r_task);

  return size * count;
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

  CHIMAERA_CLIENT_INIT();
  chi::dtio::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  
  size_t data_size = hshm::Unit<size_t>::Megabytes((size_t)ceil((double)(size * count) / 1024.0));
  size_t data_offset = offset;
  hipc::FullPtr<char> orig_data =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, data_size);
  hipc::FullPtr<char> filename_fptr =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, DTIO_FILENAME_MAX);
  char *filename_ptr = filename.ptr_;
  sprintf(filename_ptr, filename);
  char *data_ptr = orig_data.ptr_;
  memcpy(data_ptr, (const char *)buf, count);
  client.Write(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0), orig_data.shm_,
	       data_size, data_offset, filename_fptr.shm_, DTIO_FILENAME_MAX);
  
  // auto w_task = task(task_type::WRITE_TASK, source, destination);

  // w_task.iface = io_client_type::STDIO;
  // auto write_tasks = task_m->build_write_task(w_task, static_cast<const char *>(ptr));

  return size * count;
}
