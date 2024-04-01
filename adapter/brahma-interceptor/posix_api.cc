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

#include "posix_api.h"
#include "interceptor.h"
#include <dtio/common/singleton.h>
#include <dtio/drivers/posix.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>

namespace stdfs = std::filesystem;
int
dlp_open (const char *pathname, int flags, ...)
{
  mode_t mode;
  va_list args;
  long result;

  va_start (args, flags);
  if (flags & O_CREAT)
    {
      mode = va_arg (args, mode_t);
    }
  else
    {
      mode = 0;
    }
  va_end (args);
#if defined(SYS_open)
  result = syscall (SYS_open, pathname, flags, mode);
#else
  result = syscall (SYS_openat, AT_FDCWD, pathname, flags, mode);
#endif

  if (result >= 0)
    return (int)result;
  return -1;
}

ssize_t
dlp_write (int fd, const void *buf, size_t count)
{
  return syscall (SYS_write, fd, buf, count);
}

ssize_t
dlp_read (int fd, void *buf, size_t count)
{
  return syscall (SYS_read, fd, buf, count);
}

int
dlp_close (int fd)
{
  return syscall (SYS_close, fd);
}

int
dlp_fsync (int fd)
{
  return syscall (SYS_fsync, fd);
}

ssize_t
dlp_readlink (const char *path, char *buf, size_t bufsize)
{
  return syscall (SYS_readlink, path, buf, bufsize);
}

#define CATEGORY "POSIX"

std::shared_ptr<brahma::DTIOPOSIXBrahma> brahma::DTIOPOSIXBrahma::instance
    = nullptr;
int
brahma::DTIOPOSIXBrahma::open (const char *pathname, int flags, ...)
{
  BRAHMA_MAP_OR_FAIL (open);

  int ret = -1;
  if (flags & O_CREAT)
    {
      va_list args;
      va_start (args, flags);
      int mode = va_arg (args, int);
      va_end (args);

      INTERCEPT_POSIX_FUNCTION (open, pathname, flags, mode);
    }
  else
    {
      INTERCEPT_POSIX_FUNCTION (open, pathname, flags);
    }

  // if (trace)
  //   this->trace (ret);
  return ret;
}
int
brahma::DTIOPOSIXBrahma::close (int fd)
{
  BRAHMA_MAP_OR_FAIL (close);

  int ret = -1;
  INTERCEPT_POSIX_FUNCTION (close, fd);
  // int ret = __real_close (fd);

  // if (trace)
  //   this->remove_trace (fd);
  return ret;
}
ssize_t
brahma::DTIOPOSIXBrahma::write (int fd, const void *buf, size_t count)
{
  BRAHMA_MAP_OR_FAIL (write);

  ssize_t ret = -1;
  INTERCEPT_POSIX_FUNCTION (write, fd, buf, count);
  // __real_write (fd, buf, count);

  return ret;
}
ssize_t
brahma::DTIOPOSIXBrahma::read (int fd, void *buf, size_t count)
{
  BRAHMA_MAP_OR_FAIL (read);

  ssize_t ret = -1;
  INTERCEPT_POSIX_FUNCTION (read, fd, buf, count);

  // ssize_t ret = __real_read (fd, buf, count);

  return ret;
}
off_t
brahma::DTIOPOSIXBrahma::lseek (int fd, off_t offset, int whence)
{
  BRAHMA_MAP_OR_FAIL (lseek);

  ssize_t ret = -1;
  INTERCEPT_POSIX_FUNCTION (lseek, fd, offset, whence);
  // ssize_t ret = __real_lseek (fd, offset, whence);
  return ret;
}

int
brahma::DTIOPOSIXBrahma::creat64 (const char *path, mode_t mode)
{
  BRAHMA_MAP_OR_FAIL (creat64);
  int ret = __real_creat64 (path, mode);
  // if (trace)
  //   this->trace (path);
  return ret;
}

int
brahma::DTIOPOSIXBrahma::open64 (const char *path, int flags, ...)
{
  BRAHMA_MAP_OR_FAIL (open64);

  int ret = -1;
  if (flags & O_CREAT)
    {
      va_list args;
      va_start (args, flags);
      int mode = va_arg (args, int);
      va_end (args);
      INTERCEPT_POSIX_FUNCTION (open64, path, flags, mode);
      // ret = __real_open64 (path, flags, mode);
    }
  else
    {
      // ret = __real_open64 (path, flags);
      INTERCEPT_POSIX_FUNCTION (open64, path, flags);
    }

  // if (trace)
  //   this->trace (path);
  return ret;
}

off64_t
brahma::DTIOPOSIXBrahma::lseek64 (int fd, off64_t offset, int whence)
{
  BRAHMA_MAP_OR_FAIL (lseek64);
  off64_t ret = -1;
  INTERCEPT_POSIX_FUNCTION (lseek64, fd, offset, whence)
  // off64_t ret = __real_lseek64 (fd, offset, whence);
  return ret;
}

ssize_t
brahma::DTIOPOSIXBrahma::pread (int fd, void *buf, size_t count, off_t offset)
{
  BRAHMA_MAP_OR_FAIL (pread);
  // INTERCEPT_POSIX_FUNCTION(pread, fd, buf, count, offset)
  ssize_t ret = __real_pread (fd, buf, count, offset);
  return ret;
}

ssize_t
brahma::DTIOPOSIXBrahma::pread64 (int fd, void *buf, size_t count,
                                  off64_t offset)
{
  BRAHMA_MAP_OR_FAIL (pread64);
  // INTERCEPT_POSIX_FUNCTION(pread64, fd, buf, count, offset)
  ssize_t ret = __real_pread64 (fd, buf, count, offset);
  return ret;
}

ssize_t
brahma::DTIOPOSIXBrahma::pwrite (int fd, const void *buf, size_t count,
                                 off64_t offset)
{
  BRAHMA_MAP_OR_FAIL (pwrite);
  // INTERCEPT_POSIX_FUNCTION(pwrite, fd, buf, count, offset);
  ssize_t ret = __real_pwrite (fd, buf, count, offset);
  return ret;
}

ssize_t
brahma::DTIOPOSIXBrahma::pwrite64 (int fd, const void *buf, size_t count,
                                   off64_t offset)
{
  BRAHMA_MAP_OR_FAIL (pwrite64);
  // INTERCEPT_POSIX_FUNCTION(pwrite, fd, buf, count, offset);
  ssize_t ret = __real_pwrite64 (fd, buf, count, offset);
  return ret;
}

int
brahma::DTIOPOSIXBrahma::fsync (int fd)
{
  BRAHMA_MAP_OR_FAIL (fsync);
  int ret = -1; //__real_fsync (fd);
  INTERCEPT_POSIX_FUNCTION (fsync, fd);
  return ret;
}

int
brahma::DTIOPOSIXBrahma::fdatasync (int fd)
{
  BRAHMA_MAP_OR_FAIL (fdatasync);
  int ret = __real_fdatasync (fd);
  // int ret = -1; //__real_fsync (fd);
  // INTERCEPT_POSIX_FUNCTION(fsync, fd);
  return ret;
}

int
brahma::DTIOPOSIXBrahma::openat (int dirfd, const char *pathname, int flags,
                                 ...)
{
  BRAHMA_MAP_OR_FAIL (openat);
  int ret = -1;
  if (flags & O_CREAT)
    {
      va_list args;
      va_start (args, flags);
      int mode = va_arg (args, int);
      va_end (args);

      ret = __real_openat (dirfd, pathname, flags, mode);
    }
  else
    {
      ret = __real_openat (dirfd, pathname, flags);
    }

  return ret;
}

void *
brahma::DTIOPOSIXBrahma::mmap (void *addr, size_t length, int prot, int flags,
                               int fd, off_t offset)
{
  BRAHMA_MAP_OR_FAIL (mmap);

  void *ret = __real_mmap (addr, length, prot, flags, fd, offset);

  return ret;
}

void *
brahma::DTIOPOSIXBrahma::mmap64 (void *addr, size_t length, int prot,
                                 int flags, int fd, off64_t offset)
{
  BRAHMA_MAP_OR_FAIL (mmap64);

  void *ret = __real_mmap64 (addr, length, prot, flags, fd, offset);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::__xstat (int vers, const char *path, struct stat *buf)
{
  BRAHMA_MAP_OR_FAIL (__xstat);

  int ret = -1; //__real___xstat (vers, path, buf);
  INTERCEPT_POSIX_FUNCTION (__xstat, vers, path, buf);
  // int ret = __real___xstat (vers, path, buf);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::__xstat64 (int vers, const char *path,
                                    struct stat64 *buf)
{
  BRAHMA_MAP_OR_FAIL (__xstat64);

  // int ret = __real___xstat64 (vers, path, buf);
  int ret = -1; //__real___xstat (vers, path, buf);
  INTERCEPT_POSIX_FUNCTION (__xstat64, vers, path, buf);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::__lxstat (int vers, const char *path,
                                   struct stat *buf)
{
  BRAHMA_MAP_OR_FAIL (__lxstat);

  int ret = __real___lxstat (vers, path, buf);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::__lxstat64 (int vers, const char *path,
                                     struct stat64 *buf)
{
  BRAHMA_MAP_OR_FAIL (__lxstat64);

  int ret = -1;
  INTERCEPT_POSIX_FUNCTION (__lxstat64, vers, path, buf);
  // int ret = __real___lxstat64 (vers, path, buf);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::__fxstat (int vers, int fd, struct stat *buf)
{
  BRAHMA_MAP_OR_FAIL (__fxstat);

  int ret = -1;
  INTERCEPT_POSIX_FUNCTION (__fxstat, vers, fd, buf);
  // int ret = __real___fxstat (vers, fd, buf);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::__fxstat64 (int vers, int fd, struct stat64 *buf)
{
  BRAHMA_MAP_OR_FAIL (__fxstat64);

  int ret = -1;
  INTERCEPT_POSIX_FUNCTION (__fxstat64, vers, fd, buf);
  // int ret = __real___fxstat64 (vers, fd, buf);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::mkdir (const char *pathname, mode_t mode)
{
  BRAHMA_MAP_OR_FAIL (mkdir);

  int ret = __real_mkdir (pathname, mode);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::rmdir (const char *pathname)
{
  BRAHMA_MAP_OR_FAIL (rmdir);

  int ret = __real_rmdir (pathname);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::chdir (const char *path)
{
  BRAHMA_MAP_OR_FAIL (chdir);

  int ret = __real_chdir (path);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::link (const char *oldpath, const char *newpath)
{
  BRAHMA_MAP_OR_FAIL (link);

  int ret = __real_link (oldpath, newpath);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::linkat (int fd1, const char *path1, int fd2,
                                 const char *path2, int flag)
{
  BRAHMA_MAP_OR_FAIL (linkat);

  int ret = __real_linkat (fd1, path1, fd2, path2, flag);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::unlink (const char *pathname)
{
  BRAHMA_MAP_OR_FAIL (unlink);

  int ret = -1;
  INTERCEPT_POSIX_FUNCTION (unlink, pathname);
  // int ret = __real_unlink (pathname);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::symlink (const char *path1, const char *path2)
{
  BRAHMA_MAP_OR_FAIL (symlink);

  int ret = __real_symlink (path1, path2);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::symlinkat (const char *path1, int fd,
                                    const char *path2)
{
  BRAHMA_MAP_OR_FAIL (symlinkat);

  int ret = __real_symlinkat (path1, fd, path2);

  return ret;
}

ssize_t
brahma::DTIOPOSIXBrahma::readlink (const char *path, char *buf, size_t bufsize)
{
  BRAHMA_MAP_OR_FAIL (readlink);

  ssize_t ret = __real_readlink (path, buf, bufsize);

  return ret;
}

ssize_t
brahma::DTIOPOSIXBrahma::readlinkat (int fd, const char *path, char *buf,
                                     size_t bufsize)
{
  BRAHMA_MAP_OR_FAIL (readlinkat);

  ssize_t ret = __real_readlinkat (fd, path, buf, bufsize);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::rename (const char *oldpath, const char *newpath)
{
  BRAHMA_MAP_OR_FAIL (rename);

  int ret = __real_rename (oldpath, newpath);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::chmod (const char *path, mode_t mode)
{
  BRAHMA_MAP_OR_FAIL (chmod);

  int ret = __real_chmod (path, mode);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::chown (const char *path, uid_t owner, gid_t group)
{
  BRAHMA_MAP_OR_FAIL (chown);

  int ret = __real_chown (path, owner, group);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::lchown (const char *path, uid_t owner, gid_t group)
{
  BRAHMA_MAP_OR_FAIL (lchown);

  int ret = __real_lchown (path, owner, group);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::utime (const char *filename, const utimbuf *buf)
{
  BRAHMA_MAP_OR_FAIL (utime);

  int ret = __real_utime (filename, buf);

  return ret;
}

DIR *
brahma::DTIOPOSIXBrahma::opendir (const char *name)
{
  BRAHMA_MAP_OR_FAIL (opendir);

  DIR *ret = __real_opendir (name);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::fcntl (int fd, int cmd, ...)
{
  BRAHMA_MAP_OR_FAIL (fcntl);
  if (cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC || cmd == F_SETFD
      || cmd == F_SETFL || cmd == F_SETOWN)
    { // arg: int
      va_list arg;
      va_start (arg, cmd);
      int val = va_arg (arg, int);
      va_end (arg);

      int ret = __real_fcntl (fd, cmd, val);

      return ret;
    }
  else if (cmd == F_GETFD || cmd == F_GETFL || cmd == F_GETOWN)
    {

      int ret = __real_fcntl (fd, cmd);

      return ret;
    }
  else if (cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK)
    {
      va_list arg;
      va_start (arg, cmd);
      struct flock *lk = va_arg (arg, struct flock *);
      va_end (arg);

      int ret = __real_fcntl (fd, cmd, lk);

      return ret;
    }
  else
    { // assume arg: void, cmd==F_GETOWN_EX || cmd==F_SETOWN_EX ||cmd==F_GETSIG
      // || cmd==F_SETSIG)

      int ret = __real_fcntl (fd, cmd);

      return ret;
    }
}

int
brahma::DTIOPOSIXBrahma::dup (int oldfd)
{
  BRAHMA_MAP_OR_FAIL (dup);

  int ret = __real_dup (oldfd);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::dup2 (int oldfd, int newfd)
{
  BRAHMA_MAP_OR_FAIL (dup2);

  int ret = __real_dup2 (oldfd, newfd);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::mkfifo (const char *pathname, mode_t mode)
{
  BRAHMA_MAP_OR_FAIL (mkfifo);

  int ret = __real_mkfifo (pathname, mode);

  return ret;
}

mode_t
brahma::DTIOPOSIXBrahma::umask (mode_t mask)
{
  BRAHMA_MAP_OR_FAIL (umask);

  mode_t ret = __real_umask (mask);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::access (const char *path, int amode)
{
  BRAHMA_MAP_OR_FAIL (access);

  int ret = __real_access (path, amode);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::faccessat (int fd, const char *path, int amode,
                                    int flag)
{
  BRAHMA_MAP_OR_FAIL (faccessat);

  int ret = __real_faccessat (fd, path, amode, flag);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::remove (const char *pathname)
{
  BRAHMA_MAP_OR_FAIL (remove);

  int ret = __real_remove (pathname);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::truncate (const char *pathname, off_t length)
{
  BRAHMA_MAP_OR_FAIL (truncate);

  int ret = __real_truncate (pathname, length);

  return ret;
}

int
brahma::DTIOPOSIXBrahma::ftruncate (int fd, off_t length)
{
  BRAHMA_MAP_OR_FAIL (ftruncate);

  int ret = __real_ftruncate (fd, length);

  return ret;
}
