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

#include "filesystem_mdm.h"
#include "dtio.h"
#include <filesystem>

namespace dtio::adapter::fs {

bool MetadataManager::Create(const File &f,
                             std::shared_ptr<AdapterStat> &stat) {
  HILOG(kDebug, "Create metadata for file handler")
  ScopedRwWriteLock md_lock(lock_, kMDM_Create);
  if (path_to_dtio_file_.find(stat->path_) == path_to_dtio_file_.end()) {
    path_to_dtio_file_.emplace(stat->path_, std::list<File>());
  }
  path_to_dtio_file_[stat->path_].emplace_back(f);
  auto ret = dtio_file_to_stat_.emplace(f, std::move(stat));
  return ret.second;
}

bool MetadataManager::Update(const File &f, const AdapterStat &stat) {
  HILOG(kDebug, "Update metadata for file handler")
  ScopedRwWriteLock md_lock(lock_, kMDM_Update);
  auto iter = dtio_file_to_stat_.find(f);
  if (iter != dtio_file_to_stat_.end()) {
    *(*iter).second = stat;
    return true;
  } else {
    return false;
  }
}

std::list<File>* MetadataManager::Find(const std::string &path) {
  std::string canon_path = stdfs::absolute(path).string();
  ScopedRwReadLock md_lock(lock_, kMDM_Find);
  auto iter = path_to_dtio_file_.find(canon_path);
  if (iter == path_to_dtio_file_.end())
    return nullptr;
  else
    return &iter->second;
}

std::shared_ptr<AdapterStat> MetadataManager::Find(const File &f) {
  ScopedRwReadLock md_lock(lock_, kMDM_Find2);
  auto iter = dtio_file_to_stat_.find(f);
  if (iter == dtio_file_to_stat_.end())
    return nullptr;
  else
    return iter->second;
}

bool MetadataManager::Delete(const std::string &path, const File &f) {
  HILOG(kDebug, "Delete metadata for file handler")
  ScopedRwWriteLock md_lock(lock_, kMDM_Delete);
  auto iter = dtio_file_to_stat_.find(f);
  if (iter != dtio_file_to_stat_.end()) {
    dtio_file_to_stat_.erase(iter);
    auto &list = path_to_dtio_file_[path];
    auto f_iter = std::find(list.begin(), list.end(), f);
    path_to_dtio_file_[path].erase(f_iter);
    if (list.size() == 0) {
      path_to_dtio_file_.erase(path);
    }
    return true;
  } else {
    return false;
  }
}

}  // namespace dtio::adapter::fs
