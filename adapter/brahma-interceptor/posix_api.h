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

#ifndef HERMES_ADAPTER_POSIX_H
#define HERMES_ADAPTER_POSIX_H
#include "dtio/common/logger.h"
#include "real_api.h"
#include <boost/stacktrace.hpp>
#include <brahma/brahma.h>
#include <fcntl.h>
#include <iostream>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const std::string ignore_filenames[4]
    = { ".pfw", "/pipe", "/socket", "/proc/self" };

std::shared_ptr<std::set<std::string> > interception_whitelist = nullptr;
// auto interception_whitelist = std::make_shared<std::set<std::string>>();

inline bool
interceptp (std::string sourcename)
{
  if (interception_whitelist == nullptr)
    {
      return false;
    }
  else
    {
      return interception_whitelist->find (sourcename)
             != interception_whitelist->end ();
    }
}

// Define a macro for intercepting a POSIX function and logging it
// you need to define ret to the `return_type ret = -1`
#define INTERCEPT_POSIX_FUNCTION(func, ...)                                   \
  DTIO_LOG_DEBUG_RANKLESS ("Intercepted " << __func__);                       \
  std::string caller_name = "";                                               \
  if (boost::stacktrace::stacktrace ().size () > 1)                           \
    {                                                                         \
      caller_name = boost::stacktrace::detail::location_from_symbol (         \
                        boost::stacktrace::stacktrace ()[1].address ())       \
                        .name ();                                             \
    }                                                                         \
  if (interceptp (caller_name))                                               \
    {                                                                         \
      ret = dtio::posix::func (__VA_ARGS__);                                  \
    }                                                                         \
  else                                                                        \
    {                                                                         \
      ret = __real_##func (__VA_ARGS__);                                      \
    }

int dlp_open (const char *pathname, int flags, ...);

ssize_t dlp_write (int fd, const void *buf, size_t count);

ssize_t dlp_read (int fd, void *buf, size_t count);

int dlp_close (int fd);

int dlp_fsync (int fd);

ssize_t dlp_readlink (const char *path, char *buf, size_t bufsize);

inline bool
ignore_files (const char *filename)
{
  for (auto &file : ignore_filenames)
    {
      if (strstr (filename, file.c_str ()) != NULL)
        {
          return true;
        }
    }
  return false;
}

inline std::string
get_filename (int fd)
{
  char proclnk[PATH_MAX];
  char filename[PATH_MAX];
  snprintf (proclnk, PATH_MAX, "/proc/self/fd/%d", fd);
  size_t r = dlp_readlink (proclnk, filename, PATH_MAX);
  filename[r] = '\0';
  return filename;
}

inline std::pair<bool, std::string>
is_traced_common (const char *filename, const char *func,
                  const std::vector<std::string> &ignore_filename,
                  const std::vector<std::string> &track_filename)
{
  bool found = false;
  bool ignore = false;
  char resolved_path[PATH_MAX];
  char *data = realpath (filename, resolved_path);
  (void)data;
  if (ignore_files (resolved_path) || ignore_files (filename))
    {
      DTIO_LOG_INFO ("Profiler ignoring file %s for func %s", resolved_path,
                     func);
      return std::pair<bool, std::string> (false, filename);
    }
  for (const auto file : ignore_filename)
    {
      if (strstr (resolved_path, file.c_str ()) != NULL)
        {
          DTIO_LOG_INFO ("Profiler Intercepted POSIX not file %s for func %s",
                         resolved_path, func);
          ignore = true;
          break;
        }
    }
  if (!ignore)
    {
      for (const auto file : track_filename)
        {
          if (strstr (resolved_path, file.c_str ()) != NULL)
            {
              DTIO_LOG_INFO (
                  "Profiler Intercepted POSIX tracing file %s for func %s",
                  resolved_path, func);
              found = true;
              break;
            }
        }
    }
  if (!found and !ignore)
    DTIO_LOG_INFO (
        "Profiler Intercepted POSIX not tracing file %s for func %s",
        resolved_path, func);
  return std::pair<bool, std::string> (found, filename);
}

namespace brahma
{
class DTIOPOSIXBrahma : public POSIX
{
private:
  static std::shared_ptr<DTIOPOSIXBrahma> instance;
  std::unordered_set<int> tracked_fd;
  std::vector<std::string> track_filename;
  std::vector<std::string> ignore_filename;
  // std::shared_ptr<DLIOLogger> logger;

  inline std::pair<bool, std::string>
  is_traced (int fd, const char *func)
  {
    if (fd == -1)
      return std::pair<bool, std::string> (false, "");
    auto iter = tracked_fd.find (fd);
    if (iter != tracked_fd.end ())
      return std::pair<bool, std::string> (true, "");
    return is_traced (get_filename (fd).c_str (), func);
  }

  inline std::pair<bool, std::string>
  is_traced (const char *filename, const char *func)
  {
    return is_traced_common (filename, func, ignore_filename, track_filename);
  }

  inline void
  trace (int fd)
  {
    tracked_fd.insert (fd);
  }
  inline void
  remove_trace (int fd)
  {
    tracked_fd.erase (fd);
  }

public:
  DTIOPOSIXBrahma () : POSIX ()
  {
    DTIO_LOG_INFO ("POSIX class intercepted", "");
  }
  inline void
  trace (const char *filename)
  {
    char resolved_path[PATH_MAX];
    char *data = realpath (filename, resolved_path);
    (void)data;
    track_filename.push_back (resolved_path);
  }
  inline void
  untrace (const char *filename)
  {
    char resolved_path[PATH_MAX];
    char *data = realpath (filename, resolved_path);
    (void)data;
    ignore_filename.push_back (resolved_path);
  }
  ~DTIOPOSIXBrahma () override = default;
  static std::shared_ptr<DTIOPOSIXBrahma>
  get_instance ()
  {
    if (instance == nullptr)
      {
        instance = std::make_shared<DTIOPOSIXBrahma> ();
        POSIX::set_instance (instance);
      }
    return instance;
  }
  int open (const char *pathname, int flags, ...) override;
  int creat64 (const char *path, mode_t mode) override;
  int open64 (const char *path, int flags, ...) override;
  int close (int fd) override;
  ssize_t write (int fd, const void *buf, size_t count) override;
  ssize_t read (int fd, void *buf, size_t count) override;
  off_t lseek (int fd, off_t offset, int whence) override;
  off64_t lseek64 (int fd, off64_t offset, int whence) override;
  ssize_t pread (int fd, void *buf, size_t count, off_t offset) override;
  ssize_t pread64 (int fd, void *buf, size_t count, off64_t offset) override;
  ssize_t pwrite (int fd, const void *buf, size_t count,
                  off64_t offset) override;
  ssize_t pwrite64 (int fd, const void *buf, size_t count,
                    off64_t offset) override;
  int fsync (int fd) override;
  int fdatasync (int fd) override;

  int openat (int dirfd, const char *pathname, int flags, ...) override;

  void *mmap (void *addr, size_t length, int prot, int flags, int fd,
              off_t offset) override;

  void *mmap64 (void *addr, size_t length, int prot, int flags, int fd,
                off64_t offset) override;

  int __xstat (int vers, const char *path, struct stat *buf) override;

  int __xstat64 (int vers, const char *path, struct stat64 *buf) override;

  int __lxstat (int vers, const char *path, struct stat *buf) override;

  int __lxstat64 (int vers, const char *path, struct stat64 *buf) override;

  int __fxstat (int vers, int fd, struct stat *buf) override;

  int __fxstat64 (int vers, int fd, struct stat64 *buf) override;

  int mkdir (const char *pathname, mode_t mode) override;

  int rmdir (const char *pathname) override;

  int chdir (const char *path) override;

  int link (const char *oldpath, const char *newpath) override;

  int linkat (int fd1, const char *path1, int fd2, const char *path2,
              int flag) override;

  int unlink (const char *pathname) override;

  int symlink (const char *path1, const char *path2) override;

  int symlinkat (const char *path1, int fd, const char *path2) override;

  ssize_t readlink (const char *path, char *buf, size_t bufsize) override;

  ssize_t readlinkat (int fd, const char *path, char *buf,
                      size_t bufsize) override;

  int rename (const char *oldpath, const char *newpath) override;

  int chmod (const char *path, mode_t mode) override;

  int chown (const char *path, uid_t owner, gid_t group) override;

  int lchown (const char *path, uid_t owner, gid_t group) override;

  int utime (const char *filename, const utimbuf *buf) override;

  DIR *opendir (const char *name) override;

  int fcntl (int fd, int cmd, ...) override;

  int dup (int oldfd) override;

  int dup2 (int oldfd, int newfd) override;

  int mkfifo (const char *pathname, mode_t mode) override;

  mode_t umask (mode_t mask) override;

  int access (const char *path, int amode) override;

  int faccessat (int fd, const char *path, int amode, int flag) override;

  int remove (const char *pathname) override;

  int truncate (const char *pathname, off_t length) override;

  int ftruncate (int fd, off_t length) override;
};

} // namespace brahma
//
#endif
