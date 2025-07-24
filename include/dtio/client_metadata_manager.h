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

#ifndef DTIO_INCLUDE_DTIO_CLIENT_METADATA_MANAGER_H_
#define DTIO_INCLUDE_DTIO_CLIENT_METADATA_MANAGER_H_

#include <cstdio>
#include <mutex>
#include <string>
#include <unordered_map>

#include "hermes_shm/util/singleton.h"

namespace dtio {

struct FileInfo {
  std::string absolute_path;
  int flags;
  off_t current_offset;

  FileInfo() : flags(0), current_offset(0) {}
  FileInfo(const std::string& path, int f)
      : absolute_path(path), flags(f), current_offset(0) {}
};

class ClientMetadataManager {
 public:
  ClientMetadataManager() = default;
  ~ClientMetadataManager() = default;

  // POSIX file descriptor management
  void RegisterPosixFd(int fd, const std::string& absolute_path, int flags) {
    std::lock_guard<std::mutex> lock(posix_mutex_);
    posix_files_[fd] = FileInfo(absolute_path, flags);
  }

  bool IsPosixFdRegistered(int fd) const {
    std::lock_guard<std::mutex> lock(posix_mutex_);
    return posix_files_.find(fd) != posix_files_.end();
  }

  FileInfo* GetPosixFileInfo(int fd) {
    std::lock_guard<std::mutex> lock(posix_mutex_);
    auto it = posix_files_.find(fd);
    return (it != posix_files_.end()) ? &it->second : nullptr;
  }

  void UnregisterPosixFd(int fd) {
    std::lock_guard<std::mutex> lock(posix_mutex_);
    posix_files_.erase(fd);
  }

  void UpdatePosixOffset(int fd, off_t new_offset) {
    std::lock_guard<std::mutex> lock(posix_mutex_);
    auto it = posix_files_.find(fd);
    if (it != posix_files_.end()) {
      it->second.current_offset = new_offset;
    }
  }

  // STDIO file pointer management
  void RegisterStdioFp(FILE* fp, const std::string& absolute_path, int flags) {
    std::lock_guard<std::mutex> lock(stdio_mutex_);
    stdio_files_[fp] = FileInfo(absolute_path, flags);
  }

  bool IsStdioFpRegistered(FILE* fp) const {
    std::lock_guard<std::mutex> lock(stdio_mutex_);
    return stdio_files_.find(fp) != stdio_files_.end();
  }

  FileInfo* GetStdioFileInfo(FILE* fp) {
    std::lock_guard<std::mutex> lock(stdio_mutex_);
    auto it = stdio_files_.find(fp);
    return (it != stdio_files_.end()) ? &it->second : nullptr;
  }

  void UnregisterStdioFp(FILE* fp) {
    std::lock_guard<std::mutex> lock(stdio_mutex_);
    stdio_files_.erase(fp);
  }

  void UpdateStdioOffset(FILE* fp, long new_offset) {
    std::lock_guard<std::mutex> lock(stdio_mutex_);
    auto it = stdio_files_.find(fp);
    if (it != stdio_files_.end()) {
      it->second.current_offset = new_offset;
    }
  }

 private:
  mutable std::mutex posix_mutex_;
  mutable std::mutex stdio_mutex_;
  std::unordered_map<int, FileInfo> posix_files_;
  std::unordered_map<FILE*, FileInfo> stdio_files_;
};

}  // namespace dtio

// Global singleton macros
HSHM_DEFINE_GLOBAL_PTR_VAR_H(dtio::ClientMetadataManager, kDtioClientMeta);

// Convenience macro
#define DTIO_CLIENT_META HSHM_GET_GLOBAL_PTR_VAR(dtio::ClientMetadataManager, kDtioClientMeta)

#endif  // DTIO_INCLUDE_DTIO_CLIENT_METADATA_MANAGER_H_ 