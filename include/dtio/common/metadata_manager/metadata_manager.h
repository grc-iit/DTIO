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
#ifndef DTIO_MAIN_METADATA_MANAGER_H
#define DTIO_MAIN_METADATA_MANAGER_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <cereal/types/memory.hpp>
#include <cstdio>
#include <dtio/common/data_structures.h>
#include <dtio/dtio_system.h>
#include <string>
#include <unordered_map>
/******************************************************************************
 *Class
 ******************************************************************************/
class metadata_manager {
private:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  static std::shared_ptr<metadata_manager> instance;
  std::unordered_map<FILE *, std::string> fh_map;
  std::unordered_map<int, std::string> fd_map; // TODO how do we use fh_map? Is it needed?
  std::unordered_map<std::string, file_stat> file_map;
  service service_i;
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  explicit metadata_manager(service service)
      : fh_map(), file_map(), service_i(service) {}

public:
  /******************************************************************************
   *Interface
   ******************************************************************************/
  inline static std::shared_ptr<metadata_manager> getInstance(service service) {
    return instance == nullptr ? instance = std::shared_ptr<metadata_manager>(
                                     new metadata_manager(service))
                               : instance;
  }
  bool is_created(std::string filename);
  int create(std::string filename, std::string mode, FILE *&fh);
  int create(std::string filename, int flags, mode_t mode, int *fd);
  bool is_opened(std::string filename);
  bool is_opened(FILE *fh);
  bool is_opened(int fd);
  int update_on_open(std::string filename, std::string mode, FILE *&fh);
  int update_on_open(std::string filename, int flags, mode_t mode, int *fd, int existing_size = 0);
  int update_on_close(FILE *&fh);
  int update_on_close(int fd);
  int remove_chunks(std::string &basic_string);
  std::string get_filename(FILE *fh);
  std::string get_filename(int fd);
  std::size_t get_filesize(std::string basic_string);
  std::string get_mode(std::string basic_string);
  int get_flags(std::string basic_string);
  mode_t get_posix_mode(std::string basic_string);
  long long int get_fp(const std::string &basic_string);
  int update_on_seek(std::string basic_string, size_t offset, size_t origin);
  int update_read_task_info(std::vector<task> task_k,
                            std::string filename);
  int update_write_task_info(std::vector<task> task_ks,
                             std::string filename);
  int update_delete_task_info(task task_ks, std::string filename);
  int update_write_task_info(task task_ks, std::string filename,
                             std::size_t io_size);
  std::vector<chunk_meta> fetch_chunks(task task);
  void update_on_read(std::string filename, size_t size);
  void update_on_write(std::string filename, size_t size, size_t offset);
  void update_on_delete(std::string filename);
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~metadata_manager() {
    // TODO: serialize structures and send down to dtio_meta_file
  }
};

#endif // DTIO_MAIN_METADATA_MANAGER_H
