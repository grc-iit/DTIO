/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Dynamically checked to see which are the real APIs and which are intercepted
bool posix_intercepted = true;

#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
// #include "hermes_shm/util/logging.h"
#include <filesystem>

// #include "hermes_types.h"
#include "hermes_shm/util/singleton.h"
// #include "interceptor.h"
#include <dtio/drivers/posix.h>

#include "posix_api.h"
#include "posix_fs_api.h"
#include "filesystem/filesystem.h"

// using hermes::adapter::fs::AdapterStat;
// using hermes::adapter::fs::IoStatus;
// using hermes::adapter::fs::File;
// using hermes::adapter::fs::SeekMode;

namespace hapi = hermes::api;
namespace stdfs = std::filesystem;

extern "C" {

static __attribute__((constructor(101))) void init_posix(void) {
  HERMES_POSIX_API;
  HERMES_POSIX_FS;
  TRANSPARENT_HERMES;
}/**/

/**
 * POSIX
 */
int HERMES_DECL(open)(const char *path, int flags, ...) {
	dtio::posix::open(path, flags);
}

int HERMES_DECL(open64)(const char *path, int flags, ...) {
	dtio::posix::open64(path, flags);
}

int HERMES_DECL(__open_2)(const char *path, int oflag) {
//  TRANSPARENT_HERMES
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsPathTracked(path)) {
    HILOG(kDebug, "Intercept __open_2 for filename: {}"
          " and mode: {}"
          " is tracked.", path, oflag)
    AdapterStat stat;
    stat.flags_ = oflag;
    stat.st_mode_ = 0;
    return fs_api->Open(stat, path).hermes_fd_;
  }
  return real_api->__open_2(path, oflag);
}

int HERMES_DECL(creat)(const char *path, mode_t mode) {
	dtio::posix::creat(path, mode);
}

int HERMES_DECL(creat64)(const char *path, mode_t mode) {
	dtio::posix::creat64(path, mode);
}

ssize_t HERMES_DECL(read)(int fd, void *buf, size_t count) {
	dtio::posix::read(fd, buf, count);
}

ssize_t HERMES_DECL(write)(int fd, const void *buf, size_t count) {
	dtio::posix::write(fd, buf, count);
}

ssize_t HERMES_DECL(pread)(int fd, void *buf, size_t count, off_t offset) {
	dtio::posix::pread(fd, buf, count, offset);
}

ssize_t HERMES_DECL(pwrite)(int fd, const void *buf, size_t count,
                            off_t offset) {
	dtio::posix::pwrite(fd, buf, count, offset);
}

ssize_t HERMES_DECL(pread64)(int fd, void *buf, size_t count, off64_t offset) {
	dtio::posix::pread64(fd, buf, count, offset);
}

ssize_t HERMES_DECL(pwrite64)(int fd, const void *buf, size_t count,
                              off64_t offset) {
	dtio::posix::pwrite64(fd, buf, count, offset);
}

off_t HERMES_DECL(lseek)(int fd, off_t offset, int whence) {
	dtio::posix::lseek(fd, offset, whence);
}

off64_t HERMES_DECL(lseek64)(int fd, off64_t offset, int whence) {
	dtio::posix::lseek64(fd, offset, whence);
}

int HERMES_DECL(__fxstat)(int __ver, int fd, struct stat *buf) {
  int result = 0;
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsFdTracked(fd)) {
    HILOG(kDebug, "Intercepted __fxstat.")
    File f; f.hermes_fd_ = fd;
    result = fs_api->Stat(f, buf);
  } else {
    result = real_api->__fxstat(__ver, fd, buf);
  }
  return result;
}

int HERMES_DECL(__fxstatat)(int __ver, int __fildes,
                            const char *__filename,
                            struct stat *__stat_buf, int __flag) {
  int result = 0;
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsPathTracked(__filename)) {
    HILOG(kDebug, "Intercepted __fxstatat.")
    result = fs_api->Stat(__filename, __stat_buf);
  } else {
    result = real_api->__fxstatat(__ver, __fildes, __filename,
                                  __stat_buf, __flag);
  }
  return result;
}

int HERMES_DECL(__xstat)(int __ver, const char *__filename,
                         struct stat *__stat_buf) {
  int result = 0;
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsPathTracked(__filename)) {
    // HILOG(kDebug, "Intercepted __xstat for file {}.", __filename)
    result = fs_api->Stat(__filename, __stat_buf);
  } else {
    result = real_api->__xstat(__ver, __filename, __stat_buf);
  }
  return result;
}

int HERMES_DECL(__lxstat)(int __ver, const char *__filename,
                          struct stat *__stat_buf) {
  int result = 0;
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsPathTracked(__filename)) {
    HILOG(kDebug, "Intercepted __lxstat.")
    result = fs_api->Stat(__filename, __stat_buf);
  } else {
    result = real_api->__lxstat(__ver, __filename, __stat_buf);
  }
  return result;
}

int HERMES_DECL(fstat)(int fd, struct stat *buf) {
	dtio::posix::stat(fd, buf);
}

int HERMES_DECL(stat)(const char *pathname, struct stat *buf) {
	dtio::posix::stat(pathname, buf);
}

int HERMES_DECL(__fxstat64)(int __ver, int fd, struct stat64 *buf) {
  int result = 0;
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsFdTracked(fd)) {
    HILOG(kDebug, "Intercepted __fxstat.")
    File f; f.hermes_fd_ = fd;
    result = fs_api->Stat(f, buf);
  } else {
    result = real_api->__fxstat64(__ver, fd, buf);
  }
  return result;
}

int HERMES_DECL(__fxstatat64)(int __ver, int __fildes,
                            const char *__filename,
                            struct stat64 *__stat_buf, int __flag) {
  int result = 0;
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsPathTracked(__filename)) {
    HILOG(kDebug, "Intercepted __fxstatat.")
    result = fs_api->Stat(__filename, __stat_buf);
  } else {
    result = real_api->__fxstatat64(__ver, __fildes, __filename,
                                  __stat_buf, __flag);
  }
  return result;
}

int HERMES_DECL(__xstat64)(int __ver, const char *__filename,
                         struct stat64 *__stat_buf) {
  int result = 0;
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsPathTracked(__filename)) {
    HILOG(kDebug, "Intercepted __xstat.")
    result = fs_api->Stat(__filename, __stat_buf);
  } else {
    result = real_api->__xstat64(__ver, __filename, __stat_buf);
  }
  return result;
}

int HERMES_DECL(__lxstat64)(int __ver, const char *__filename,
                          struct stat64 *__stat_buf) {
  int result = 0;
  auto real_api = HERMES_POSIX_API;
  auto fs_api = HERMES_POSIX_FS;
  if (fs_api->IsPathTracked(__filename)) {
    HILOG(kDebug, "Intercepted __lxstat.")
    result = fs_api->Stat(__filename, __stat_buf);
  } else {
    result = real_api->__lxstat64(__ver, __filename, __stat_buf);
  }
  return result;
}

int HERMES_DECL(fstat64)(int fd, struct stat64 *buf) {
	dtio::posix::fstat64(fd, buf);
}

int HERMES_DECL(stat64)(const char *pathname, struct stat64 *buf) {
	dtio::posix::stat64(pathname, buf);
}

int HERMES_DECL(fsync)(int fd) {
	dtio::posix::fsync(fd);
}

int HERMES_DECL(close)(int fd) {
	dtio::posix::close(fd);
}

int HERMES_DECL(flock)(int fd, int operation) {
	dtio::posix::flock(fd, operation);
}

int HERMES_DECL(remove)(const char *pathname) {
	dtio::posix::remove(pathname);
}

int HERMES_DECL(unlink)(const char *pathname) {
	dtio::posix::unlink(pathname);
}

}  // extern C
