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

#ifndef DTIO_ADAPTER_METADATA_MANAGER_H
#define DTIO_ADAPTER_METADATA_MANAGER_H

#include <cstdio>
#include <unordered_map>
#include "filesystem_io_client.h"
#include "filesystem.h"
#include "thread_pool.h"

namespace dtio::adapter::fs {

/**
 * Metadata manager for POSIX adapter
 */
class MetadataManager {
 private:
  std::unordered_map<std::string, std::list<File>>
      path_to_dtio_file_; /**< Map to determine if path is buffered. */
  std::unordered_map<File, std::shared_ptr<AdapterStat>>
      dtio_file_to_stat_; /**< Map for metadata */
  RwLock lock_;             /**< Lock to synchronize MD updates*/

 public:
  /** map for Dtio request */
  std::unordered_map<uint64_t, DtioRequest*> request_map;
  FsIoClientMetadata fs_mdm_; /**< Context needed for I/O clients */

  /** Constructor */
  MetadataManager() = default;

  /** Get the current adapter mode */
  AdapterMode GetBaseAdapterMode() {
    ScopedRwReadLock md_lock(lock_, kFS_GetBaseAdapterMode);
    return DTIO->client_config_.GetBaseAdapterMode();
  }

  /** Get the adapter mode for a particular file */
  AdapterMode GetAdapterMode(const std::string &path) {
    ScopedRwReadLock md_lock(lock_, kFS_GetAdapterMode);
    return DTIO->client_config_.GetAdapterConfig(path).mode_;
  }

  /** Get the adapter page size for a particular file */
  size_t GetAdapterPageSize(const std::string &path) {
    ScopedRwReadLock md_lock(lock_, kFS_GetAdapterPageSize);
    return DTIO->client_config_.GetAdapterConfig(path).page_size_;
  }

  /**
   * Create a metadata entry for filesystem adapters given File handler.
   * @param f original file handler of the file on the destination
   * filesystem.
   * @param stat POSIX Adapter version of Stat data structure.
   * @return    true, if operation was successful.
   *            false, if operation was unsuccessful.
   */
  bool Create(const File& f, std::shared_ptr<AdapterStat> &stat);

  /**
   * Update existing metadata entry for filesystem adapters.
   * @param f original file handler of the file on the destination.
   * @param stat POSIX Adapter version of Stat data structure.
   * @return    true, if operation was successful.
   *            false, if operation was unsuccessful or entry doesn't exist.
   */
  bool Update(const File& f, const AdapterStat& stat);

  /**
   * Delete existing metadata entry for for filesystem adapters.
   * @param f original file handler of the file on the destination.
   * @return    true, if operation was successful.
   *            false, if operation was unsuccessful.
   */
  bool Delete(const std::string &path, const File& f);

  /**
   * Find the dtio file relating to a path.
   * @param path the path being checked
   * @return The dtio file.
   * */
  std::list<File>* Find(const std::string &path);

  /**
   * Find existing metadata entry for filesystem adapters.
   * @param f original file handler of the file on the destination.
   * @return    The metadata entry if exist.
   *            The bool in pair indicated whether metadata entry exists.
   */
  std::shared_ptr<AdapterStat> Find(const File& f);
};
}  // namespace dtio::adapter::fs

// Singleton macros
#include "dtio_shm/util/singleton.h"

#define DTIO_FS_METADATA_MANAGER \
  hshm::Singleton<dtio::adapter::fs::MetadataManager>::GetInstance()
#define DTIO_FS_METADATA_MANAGER_T dtio::adapter::fs::MetadataManager*

#define DTIO_FS_THREAD_POOL \
  hshm::EasySingleton<dtio::ThreadPool>::GetInstance()

#endif  // DTIO_ADAPTER_METADATA_MANAGER_H
