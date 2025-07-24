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

// Dynamically checked to see which are the real APIs and which are intercepted
bool posix_intercepted = true;

// Prevent conflicts with system 64-bit function definitions
// #ifndef _LARGEFILE64_SOURCE
// #define _LARGEFILE64_SOURCE 1
// #endif
// #ifndef _GNU_SOURCE
// #define _GNU_SOURCE 1
// #endif

#include "posix_api.h"

#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>

#include "dtio/client_metadata_manager.h"
#include "dtio/config_manager.h"
#include "dtio/dtio_enumerations.h"
#include "interceptor.h"

namespace stdfs = std::filesystem;

extern "C" {

static __attribute__((constructor(101))) void init_posix(void) {}

/**
 * MPI
 */
int HERMES_DECL(MPI_Init)(int *argc, char ***argv) {
  return 0;  // Success
}

int HERMES_DECL(MPI_Init_thread)(int *argc, char ***argv, int required,
                                 int *provided) {
  return 0;  // Success
}

/**
 * POSIX
 */
#if !defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64
int HERMES_DECL(open)(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = va_arg(args, mode_t);
    va_end(args);
  }

  // Call real open first
  int real_fd;
  if (flags & O_CREAT) {
    real_fd = HERMES_POSIX_API->open(path, flags, mode);
  } else {
    real_fd = HERMES_POSIX_API->open(path, flags);
  }

  if (real_fd < 0) {
    return real_fd;
  }

  // Convert to absolute path
  std::string abs_path = stdfs::absolute(path).string();

  // Check if should intercept
  auto *config = DTIO_CONF;
  if (!config->ShouldIntercept(abs_path)) {
    return real_fd;
  }

  // Register with metadata manager
  auto *client_meta = DTIO_CLIENT_META;
  client_meta->RegisterPosixFd(real_fd, abs_path, flags);

  return real_fd;
}
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
int HERMES_DECL(open64)(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = va_arg(args, mode_t);
    va_end(args);
  }

  // Call real open64 first
  int real_fd;
  if (flags & O_CREAT) {
    real_fd = HERMES_POSIX_API->open64(path, flags, mode);
  } else {
    real_fd = HERMES_POSIX_API->open64(path, flags);
  }

  if (real_fd < 0) {
    return real_fd;
  }

  // Convert to absolute path
  std::string abs_path = stdfs::absolute(path).string();

  // Check if should intercept
  auto *config = DTIO_CONF;
  if (!config->ShouldIntercept(abs_path)) {
    return real_fd;
  }

  // Register with metadata manager
  auto *client_meta = DTIO_CLIENT_META;
  client_meta->RegisterPosixFd(real_fd, abs_path, flags);

  return real_fd;
}
#endif

int HERMES_DECL(__open_2)(const char *path, int oflag) {
  return -1;  // Not implemented
}

#if !defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64
int HERMES_DECL(creat)(const char *path, mode_t mode) {
  // creat is equivalent to open(path, O_CREAT | O_WRONLY | O_TRUNC, mode)
  int flags = O_CREAT | O_WRONLY | O_TRUNC;

  // Call real creat first
  int real_fd = HERMES_POSIX_API->creat(path, mode);

  if (real_fd < 0) {
    return real_fd;
  }

  // Convert to absolute path
  std::string abs_path = stdfs::absolute(path).string();

  // Check if should intercept
  auto *config = DTIO_CONF;
  if (!config->ShouldIntercept(abs_path)) {
    return real_fd;
  }

  // Register with metadata manager
  auto *client_meta = DTIO_CLIENT_META;
  client_meta->RegisterPosixFd(real_fd, abs_path, flags);

  return real_fd;
}
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
int HERMES_DECL(creat64)(const char *path, mode_t mode) {
  // creat64 is equivalent to open64(path, O_CREAT | O_WRONLY | O_TRUNC, mode)
  int flags = O_CREAT | O_WRONLY | O_TRUNC;

  // Call real creat64 first
  int real_fd = HERMES_POSIX_API->creat64(path, mode);

  if (real_fd < 0) {
    return real_fd;
  }

  // Convert to absolute path
  std::string abs_path = stdfs::absolute(path).string();

  // Check if should intercept
  auto *config = DTIO_CONF;
  if (!config->ShouldIntercept(abs_path)) {
    return real_fd;
  }

  // Register with metadata manager
  auto *client_meta = DTIO_CLIENT_META;
  client_meta->RegisterPosixFd(real_fd, abs_path, flags);

  return real_fd;
}
#endif

ssize_t HERMES_DECL(read)(int fd, void *buf, size_t count) {
  // Check if file descriptor is registered with DTIO
  auto *client_meta = DTIO_CLIENT_META;
  if (!client_meta->IsPosixFdRegistered(fd)) {
    // Not intercepted, call real API
    return HERMES_POSIX_API->read(fd, buf, count);
  }

  // Get file info and submit to DTIO runtime
  auto *file_info = client_meta->GetPosixFileInfo(fd);
  if (file_info) {
    auto *config = DTIO_CONF;

    // Allocate buffer in shared memory
    hipc::FullPtr<char> shm_buf = CHI_CLIENT->AllocateBuffer(HSHM_MCTX, count);

    // Submit read task with filename as chi::string
    config->dtio_mod_.Read(
        HSHM_MCTX, shm_buf.shm_, count, file_info->current_offset,
        chi::string(file_info->absolute_path), dtio::IoClientType::kPosix);

    // Copy data back to user buffer
    memcpy(buf, shm_buf.ptr_, count);

    // Update offset
    client_meta->UpdatePosixOffset(fd, file_info->current_offset + count);

    // Free shared memory buffer
    CHI_CLIENT->FreeBuffer(HSHM_MCTX, shm_buf);

    return count;
  }

  // Fallback to real API
  return HERMES_POSIX_API->read(fd, buf, count);
}

ssize_t HERMES_DECL(write)(int fd, const void *buf, size_t count) {
  // Check if file descriptor is registered with DTIO
  auto *client_meta = DTIO_CLIENT_META;
  if (!client_meta->IsPosixFdRegistered(fd)) {
    // Not intercepted, call real API
    return HERMES_POSIX_API->write(fd, buf, count);
  }

  // Get file info and submit to DTIO runtime
  auto *file_info = client_meta->GetPosixFileInfo(fd);
  if (file_info) {
    auto *config = DTIO_CONF;

    // Allocate buffer in shared memory
    hipc::FullPtr<char> shm_buf = CHI_CLIENT->AllocateBuffer(HSHM_MCTX, count);
    memcpy(shm_buf.ptr_, buf, count);

    // Submit write task with filename as chi::string
    config->dtio_mod_.Write(
        HSHM_MCTX, shm_buf.shm_, count, file_info->current_offset,
        chi::string(file_info->absolute_path), dtio::IoClientType::kPosix);

    // Update offset
    client_meta->UpdatePosixOffset(fd, file_info->current_offset + count);

    // Free shared memory buffer
    CHI_CLIENT->FreeBuffer(HSHM_MCTX, shm_buf);

    return count;
  }

  // Fallback to real API
  return HERMES_POSIX_API->write(fd, buf, count);
}

#if !defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64
off_t HERMES_DECL(lseek)(int fd, off_t offset, int whence) {
  // Check if file descriptor is registered with DTIO
  auto *client_meta = DTIO_CLIENT_META;
  if (!client_meta->IsPosixFdRegistered(fd)) {
    // Not intercepted, call real API
    return HERMES_POSIX_API->lseek(fd, offset, whence);
  }

  // Call real lseek first to get the actual position
  off_t real_offset = HERMES_POSIX_API->lseek(fd, offset, whence);
  if (real_offset < 0) {
    return real_offset;
  }

  // Update DTIO metadata with new offset
  auto *file_info = client_meta->GetPosixFileInfo(fd);
  if (file_info) {
    client_meta->UpdatePosixOffset(fd, real_offset);
  }

  return real_offset;
}
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
off64_t HERMES_DECL(lseek64)(int fd, off64_t offset, int whence) {
  // Check if file descriptor is registered with DTIO
  auto *client_meta = DTIO_CLIENT_META;
  if (!client_meta->IsPosixFdRegistered(fd)) {
    // Not intercepted, call real API
    return HERMES_POSIX_API->lseek64(fd, offset, whence);
  }

  // Call real lseek64 first to get the actual position
  off64_t real_offset = HERMES_POSIX_API->lseek64(fd, offset, whence);
  if (real_offset < 0) {
    return real_offset;
  }

  // Update DTIO metadata with new offset
  auto *file_info = client_meta->GetPosixFileInfo(fd);
  if (file_info) {
    client_meta->UpdatePosixOffset(fd, real_offset);
  }

  return real_offset;
}
#endif

int HERMES_DECL(__fxstat)(int __ver, int fd, struct stat *buf) {
  return -1;  // Not implemented
}

int HERMES_DECL(__fxstatat)(int __ver, int __fildes, const char *__filename,
                            struct stat *__stat_buf, int __flag) {
  return -1;  // Not implemented
}

int HERMES_DECL(__xstat)(int __ver, const char *__filename,
                         struct stat *__stat_buf) {
  return -1;  // Not implemented
}

int HERMES_DECL(__lxstat)(int __ver, const char *__filename,
                          struct stat *__stat_buf) {
  return -1;  // Not implemented
}

#if !defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64
int HERMES_DECL(stat)(const char *pathname, struct stat *buf) {
  // For stat, we don't need special DTIO handling - just call real API
  // The file doesn't need to be open for stat operations
  return HERMES_POSIX_API->stat(pathname, buf);
}
#endif

int HERMES_DECL(__fxstat64)(int __ver, int fd, struct stat64 *buf) {
  return -1;  // Not implemented
}

int HERMES_DECL(__fxstatat64)(int __ver, int __fildes, const char *__filename,
                              struct stat64 *__stat_buf, int __flag) {
  return -1;  // Not implemented
}

int HERMES_DECL(__xstat64)(int __ver, const char *__filename,
                           struct stat64 *__stat_buf) {
  return -1;  // Not implemented
}

int HERMES_DECL(__lxstat64)(int __ver, const char *__filename,
                            struct stat64 *__stat_buf) {
  return -1;  // Not implemented
}

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
int HERMES_DECL(stat64)(const char *pathname, struct stat64 *buf) {
  // For stat64, we don't need special DTIO handling - just call real API
  // The file doesn't need to be open for stat operations
  return HERMES_POSIX_API->stat64(pathname, buf);
}
#endif

int HERMES_DECL(fsync)(int fd) {
  // Check if file descriptor is registered with DTIO
  auto *client_meta = DTIO_CLIENT_META;
  if (!client_meta->IsPosixFdRegistered(fd)) {
    // Not intercepted, call real API
    return HERMES_POSIX_API->fsync(fd);
  }

  // For DTIO-managed files, call real fsync
  // In the future, this could trigger DTIO-specific flush operations
  return HERMES_POSIX_API->fsync(fd);
}

int HERMES_DECL(close)(int fd) {
  // Check if file descriptor is registered with DTIO
  auto *client_meta = DTIO_CLIENT_META;
  if (!client_meta->IsPosixFdRegistered(fd)) {
    // Not intercepted, call real API
    return HERMES_POSIX_API->close(fd);
  }

  // Unregister from DTIO metadata manager
  client_meta->UnregisterPosixFd(fd);

  // Call real close
  return HERMES_POSIX_API->close(fd);
}

int HERMES_DECL(unlink)(const char *pathname) {
  // For unlink, we don't need special DTIO handling - just call real API
  // File removal doesn't require tracking in DTIO metadata
  return HERMES_POSIX_API->unlink(pathname);
}

// NOTE : not in DTIO ssize_t HERMES_DECL(pread)(int fd, void *buf, size_t
// count,
//                                               off_t offset) {
//   printf("pread called\n");
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
//   // dtio::posix::pread(fd, buf, count, offset);
// }

// ssize_t HERMES_DECL(pwrite)(int fd, const void *buf, size_t count,
//                             off_t offset) {
//   printf("pwrite called\n");
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
//   // dtio::posix::pwrite(fd, buf, count, offset);
// }

// ssize_t HERMES_DECL(pread64)(int fd, void *buf, size_t count, off64_t offset)
// {
//   printf("pread64 called\n");
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
//   // dtio::posix::pread64(fd, buf, count, offset);
// }

// ssize_t HERMES_DECL(pwrite64)(int fd, const void *buf, size_t count,
//                               off64_t offset) {
//   printf("pwrite64 called\n");
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
//   // dtio::posix::pwrite64(fd, buf, count, offset);
// }

// NOTE:not in DTIO
// int HERMES_DECL(fstat)(int fd, struct stat *buf) {
//   printf("fstat called\n");
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
//   auto real_api = HERMES_POSIX_API;

//   std::string caller_name = "";

//   if (real_api->fd_whitelistedp(fd)) {
//     return dtio::posix::myfstat(fd, buf);
//   }
//   else if (ConfigManager::get_instance()->NEVER_TRACE) {
//     return real_api->fstat(fd, buf);
//   }

//   if (boost::stacktrace::stacktrace().size() > 1) {
//     caller_name =
//     boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
//   }
//   if (real_api->interceptp(caller_name)) {
//     return dtio::posix::myfstat(fd, buf);
//   }
//   else {
//     return real_api->fstat(fd, buf);
//   }
// }

// NOTE: not in DTIO
// int HERMES_DECL(fstat64)(int fd, struct stat64 *buf) {
//   printf("fstat64 called\n");
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
//   auto real_api = HERMES_POSIX_API;

//   std::string caller_name = "";

//   if (real_api->fd_whitelistedp(fd)) {
//     return dtio::posix::myfstat64(fd, buf);
//   }
//   else if (ConfigManager::get_instance()->NEVER_TRACE) {
//     return real_api->fstat64(fd, buf);
//   }

//   if (boost::stacktrace::stacktrace().size() > 1) {
//     caller_name =
//     boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
//   }
//   if (real_api->interceptp(caller_name)) {
//     return dtio::posix::myfstat64(fd, buf);
//   }
//   else {
//     return real_api->fstat64(fd, buf);
//   }
// }

// int HERMES_DECL(flock)(int fd, int operation) {
//   printf("flock called\n");
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
//   // FIXME For now, we're ignoring this call and hoping it doesnt impact
//   program logic
//   // dtio::posix::flock(fd, operation);
// }

// int HERMES_DECL(remove)(const char *pathname) {
//   printf("remove called\n");
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
//   // FIXME For now, we're ignoring this call and hoping it doesnt impact
//   program logic
//   // dtio::posix::remove(pathname);
// }

}  // extern C
