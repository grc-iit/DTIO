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

#include "posix_io_client.h"

namespace hermes::adapter::fs {

/** Allocate an fd for the file f */
void PosixIoClient::RealOpen(File &f,
                             AdapterStat &stat,
                             const std::string &path) {
  if (stat.flags_ & O_APPEND) {
    stat.hflags_.SetBits(HERMES_FS_APPEND);
  }
  if (stat.flags_ & O_CREAT || stat.flags_ & O_TMPFILE) {
    stat.hflags_.SetBits(HERMES_FS_CREATE);
  }
  if (stat.flags_ & O_TRUNC) {
    stat.hflags_.SetBits(HERMES_FS_TRUNC);
  }

  if (stat.hflags_.Any(HERMES_FS_CREATE)) {
    if (stat.adapter_mode_ != AdapterMode::kScratch) {
      stat.fd_ = real_api->open(path.c_str(), stat.flags_, stat.st_mode_);
    }
  } else {
    stat.fd_ = real_api->open(path.c_str(), stat.flags_);
  }

  if (stat.fd_ >= 0) {
    stat.hflags_.SetBits(HERMES_FS_EXISTS);
  }
  if (stat.fd_ < 0 && stat.adapter_mode_ != AdapterMode::kScratch) {
    f.status_ = false;
  }
}

/**
 * Called after real open. Allocates the Hermes representation of
 * identifying file information, such as a hermes file descriptor
 * and hermes file handler. These are not the same as POSIX file
 * descriptor and STDIO file handler.
 * */
void PosixIoClient::HermesOpen(File &f,
                               const AdapterStat &stat,
                               FilesystemIoClientState &fs_mdm) {
  f.hermes_fd_ = fs_mdm.mdm_->AllocateFd();
}

/** Synchronize \a file FILE f */
int PosixIoClient::RealSync(const File &f,
                            const AdapterStat &stat) {
  (void) f;
  if (stat.adapter_mode_ == AdapterMode::kScratch &&
      stat.fd_ == -1) {
    return 0;
  }
  return real_api->fsync(stat.fd_);
}

/** Close \a file FILE f */
int PosixIoClient::RealClose(const File &f,
                             AdapterStat &stat) {
  (void) f;
  if (stat.adapter_mode_ == AdapterMode::kScratch &&
      stat.fd_ == -1) {
    return 0;
  }
  return real_api->close(stat.fd_);
}

/**
 * Called before RealClose. Releases information provisioned during
 * the allocation phase.
 * */
void PosixIoClient::HermesClose(File &f,
                               const AdapterStat &stat,
                               FilesystemIoClientState &fs_mdm) {
  fs_mdm.mdm_->ReleaseFd(f.hermes_fd_);
}

/** Remo
 * ve \a file FILE f */
int PosixIoClient::RealRemove(const std::string &path) {
  return real_api->remove(path.c_str());
}

/** Get initial statistics from the backend */
size_t PosixIoClient::GetSize(const hipc::charbuf &bkt_name) {
  size_t true_size = 0;
  std::string filename = bkt_name.str();
  int fd = real_api->open(filename.c_str(), O_RDONLY);
  if (fd < 0) { return 0; }
  struct stat buf;
  real_api->fstat(fd, &buf);
  true_size = buf.st_size;
  real_api->close(fd);

  HILOG(kDebug, "The size of the file {} on disk is {}",
        filename, true_size)
  return true_size;
}

/** Write blob to backend */
void PosixIoClient::WriteBlob(const std::string &bkt_name,
                              const Blob &full_blob,
                              const FsIoOptions &opts,
                              IoStatus &status) {
  (void) opts;
  status.success_ = true;
  HILOG(kDebug, "Writing to file: {}"
        " offset: {}"
        " size: {}",
        bkt_name, opts.backend_off_, full_blob.size())
  int fd = real_api->open(bkt_name.c_str(), O_RDWR | O_CREAT);
  if (fd < 0) {
    status.size_ = 0;
    status.success_ = false;
    return;
  }
  status.size_ = real_api->pwrite(fd,
                                  full_blob.data(),
                                  full_blob.size(),
                                  opts.backend_off_);
  if (status.size_ != full_blob.size()) {
    status.success_ = false;
  }
  real_api->close(fd);
}

/** Read blob from the backend */
void PosixIoClient::ReadBlob(const std::string &bkt_name,
                             Blob &full_blob,
                             const FsIoOptions &opts,
                             IoStatus &status) {
  (void) opts;
  status.success_ = true;
  HILOG(kDebug, "Reading from file: {}"
        " offset: {}"
        " size: {}",
        bkt_name, opts.backend_off_, full_blob.size())
  int fd = real_api->open(bkt_name.c_str(), O_RDONLY);
  if (fd < 0) {
    status.size_ = 0;
    status.success_ = false;
    return;
  }
  status.size_ = real_api->pread(fd,
                                 full_blob.data(),
                                 full_blob.size(),
                                 opts.backend_off_);
  if (status.size_ != full_blob.size()) {
    status.success_ = false;
  }
  real_api->close(fd);
}

}  // namespace hermes::adapter::fs

HERMES_TRAIT_CC(hermes::adapter::fs::PosixIoClient)
