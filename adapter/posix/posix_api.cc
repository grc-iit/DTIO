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
// #include <filesystem>

// #include "hermes_types.h"
#include <dtio/common/singleton.h>
#include "interceptor.h"
#include <dtio/drivers/posix.h>
// #include <dtio/drivers/mpi.h>

#include "posix_api.h"
// #include "posix_fs_api.h"
// #include "filesystem/filesystem.h"

// using hermes::adapter::fs::AdapterStat;
// using hermes::adapter::fs::IoStatus;
// using hermes::adapter::fs::File;
// using hermes::adapter::fs::SeekMode;

// namespace hapi = hermes::api;
namespace stdfs = std::filesystem;

std::shared_ptr<dtio::posix::PosixApi> dtio::posix::PosixApi::instance = nullptr;

extern "C" {

static __attribute__((constructor(101))) void init_posix(void) {
	HERMES_POSIX_API;
	// HERMES_POSIX_FS;
	// TRANSPARENT_HERMES;
}/**/

/**
 * MPI
 */
int HERMES_DECL(MPI_Init)(int *argc, char ***argv) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  // You shouldn't ever be calling the real version

  std::string execname = std::string(*argv[0]);
  DTIO_LOG_DEBUG_RANKLESS("Now intercepting executable: " << execname);
  real_api->add_to_whitelist(execname);
  return dtio::MPI_Init(argc, argv);
}
// void HERMES_DECL(MPI_Finalize)() {
//   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
//   // You shouldn't ever be calling the real version

//   return dtio::MPI_Finalize();
// }

/**
 * POSIX
 */
int HERMES_DECL(open)(const char *path, int flags, ...) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;

  int mode = 0;
  if (flags & O_CREAT || flags & O_TMPFILE) {
    va_list arg;
    va_start(arg, flags);
    mode = va_arg(arg, int);
    va_end(arg);
  }

  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
  caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::open(path, flags);
  }
  else {
    if (flags & O_CREAT || flags & O_TMPFILE) {
      return real_api->open(path, flags, mode);
    }
    else {
      return real_api->open(path, flags);
    }
  }
}

int HERMES_DECL(open64)(const char *path, int flags, ...) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;

  int mode = 0;
  if (flags & O_CREAT) {
    va_list arg;
    va_start(arg, flags);
    mode = va_arg(arg, int);
    va_end(arg);
  }

  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::open64(path, flags);
  }
  else {
    if (flags & O_CREAT) {
      return real_api->open64(path, flags, mode);
    }
    else {
      return real_api->open64(path, flags);
    }
  }
}

int HERMES_DECL(__open_2)(const char *path, int oflag) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::__open_2(path, oflag);
  }
  else {
    return real_api->__open_2(path, oflag);
  }
}

// int HERMES_DECL(creat)(const char *path, mode_t mode) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::creat(path, mode);
// }

// int HERMES_DECL(creat64)(const char *path, mode_t mode) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::creat64(path, mode);
// }

ssize_t HERMES_DECL(read)(int fd, void *buf, size_t count) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  auto test = real_api->interceptp(caller_name);
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::read(fd, buf, count);
  }
  else {
    return real_api->read(fd, buf, count);
  }
}

ssize_t HERMES_DECL(write)(int fd, const void *buf, size_t count) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::write(fd, buf, count);
  }
  else {
    return real_api->write(fd, buf, count);
  }
}

// NOTE: not in DTIO
// ssize_t HERMES_DECL(pread)(int fd, void *buf, size_t count, off_t offset) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::pread(fd, buf, count, offset);
// }
//
// ssize_t HERMES_DECL(pwrite)(int fd, const void *buf, size_t count,
// 			    off_t offset) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::pwrite(fd, buf, count, offset);
// }
//
// ssize_t HERMES_DECL(pread64)(int fd, void *buf, size_t count, off64_t offset) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::pread64(fd, buf, count, offset);
// }
//
// ssize_t HERMES_DECL(pwrite64)(int fd, const void *buf, size_t count,
// 			      off64_t offset) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::pwrite64(fd, buf, count, offset);
// }

off_t HERMES_DECL(lseek)(int fd, off_t offset, int whence) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::lseek(fd, offset, whence);
  }
  else {
    return real_api->lseek(fd, offset, whence);
  }
}

off64_t HERMES_DECL(lseek64)(int fd, off64_t offset, int whence) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::lseek64(fd, offset, whence);
  }
  else {
    return real_api->lseek64(fd, offset, whence);
  }
}

int HERMES_DECL(__fxstat)(int __ver, int fd, struct stat *buf) {
	// int result = 0;
	// auto real_api = HERMES_POSIX_API;
	// auto fs_api = HERMES_POSIX_FS;
	// if (fs_api->IsFdTracked(fd)) {
	//   HILOG(kDebug, "Intercepted __fxstat.")
	//   File f; f.hermes_fd_ = fd;
	//   result = fs_api->Stat(f, buf);
	// } else {
	//   result = real_api->__fxstat(__ver, fd, buf);
	// }
	// return result;
  DTIO_LOG_DEBUG_RANKLESS("Intercepted __fxstat.");
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::__fxstat(__ver, fd, buf);
  }
  else {
    return real_api->__fxstat(__ver, fd, buf);
  }
}

int HERMES_DECL(__fxstatat)(int __ver, int __fildes,
			    const char *__filename,
			    struct stat *__stat_buf, int __flag) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::__fxstatat(__ver, __fildes, __filename, __stat_buf, __flag);
  }
  else {
    return real_api->__fxstatat(__ver, __fildes, __filename, __stat_buf, __flag);
  }
}

int HERMES_DECL(__xstat)(int __ver, const char *__filename,
			 struct stat *__stat_buf) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::__xstat(__ver, __filename, __stat_buf);
  }
  else {
    return real_api->__xstat(__ver, __filename, __stat_buf);
  }
}

int HERMES_DECL(__lxstat)(int __ver, const char *__filename,
			  struct stat *__stat_buf) {
}
// NOTE:not in DTIO
// int HERMES_DECL(fstat)(int fd, struct stat *buf) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::stat(fd, buf);
// }

int HERMES_DECL(stat)(const char *pathname, struct stat *buf) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::mystat(pathname, buf);
  }
  else {
    return real_api->stat(pathname, buf);
  }
}

int HERMES_DECL(__fxstat64)(int __ver, int fd, struct stat64 *buf) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::__fxstat64(__ver, fd, buf);
  }
  else {
    return real_api->__fxstat64(__ver, fd, buf);
  }
}

int HERMES_DECL(__fxstatat64)(int __ver, int __fildes,
			      const char *__filename,
			      struct stat64 *__stat_buf, int __flag) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::__fxstatat64(__ver, __fildes, __filename, __stat_buf, __flag);
  }
  else {
    return real_api->__fxstatat64(__ver, __fildes, __filename, __stat_buf, __flag);
  }
}

int HERMES_DECL(__xstat64)(int __ver, const char *__filename,
			   struct stat64 *__stat_buf) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::__xstat64(__ver, __filename, __stat_buf);
  }
  else {
    return real_api->__xstat64(__ver, __filename, __stat_buf);
  }
}

int HERMES_DECL(__lxstat64)(int __ver, const char *__filename,
			    struct stat64 *__stat_buf) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::__lxstat64(__ver, __filename, __stat_buf);
  }
  else {
    return real_api->__lxstat64(__ver, __filename, __stat_buf);
  }
}

// NOTE: not in DTIO
// int HERMES_DECL(fstat64)(int fd, struct stat64 *buf) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::fstat64(fd, buf);
// }

int HERMES_DECL(stat64)(const char *pathname, struct stat64 *buf) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::mystat64(pathname, buf);
  }
  else {
    return real_api->stat64(pathname, buf);
  }
}


int HERMES_DECL(fsync)(int fd) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::fsync(fd);
  }
  else {
    return real_api->fsync(fd);
  }
}

int HERMES_DECL(close)(int fd) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::close(fd);
  }
  else {
    return real_api->close(fd);
  }
}

// int HERMES_DECL(flock)(int fd, int operation) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::flock(fd, operation);
// }
//
// int HERMES_DECL(remove)(const char *pathname) {
// 	DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__)
// 	dtio::posix::remove(pathname);
// }

int HERMES_DECL(unlink)(const char *pathname) {
  DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  auto real_api = HERMES_POSIX_API;
  std::string caller_name = "";
  if (boost::stacktrace::stacktrace().size() > 1) {
    caller_name = boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  }
  if (real_api->interceptp(caller_name)) {
    return dtio::posix::unlink(pathname);
  }
  else {
    return real_api->unlink(pathname);
  }
}

}  // extern C
