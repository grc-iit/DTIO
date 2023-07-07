/*
 * Copyright (C) 2023  SCS Lab <scs-help@cs.iit.edu>, Hariharan
 * Devarajan <hdevarajan@hawk.iit.edu>, Anthony Kougkas
 * <akougkas@iit.edu>, Xian-He Sun <sun@iit.edu>
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
/*******************************************************************************
 * Created by hariharan on 2/16/18.
 * Updated by akougkas on 6/26/2018
 ******************************************************************************/
#ifndef DTIO_MAIN_POSIX_H
#define DTIO_MAIN_POSIX_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <cstdio>
#include <cstring>
#include <dtio/common/data_manager/data_manager.h>
#include <dtio/common/metadata_manager/metadata_manager.h>
#include <dtio/drivers/mpi.h>
#include <dtio/dtio_system.h>
/******************************************************************************
 *DTIO Namespace
 ******************************************************************************/
namespace dtio {
/******************************************************************************
 *Interface
 ******************************************************************************/
int open(const char *filename, int flags);
int open(const char *filename, int flags, mode_t mode);
int open64(const char *filename, int flags);
int open64(const char *filename, int flags, mode_t mode);

// Not present in Labios, but needed for POSIX emulation in IOR
// FIXME POSIX unlink is currently commented to avoid problems in HCL
// int unlink(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int stat(const char *pathname, struct stat *statbuf);
int mknod(const char *pathname, mode_t mode, dev_t dev);
// int fcntl(int fd, int cmd, ...);

int close(int fd);
off_t lseek(int fd, off_t offset, int whence);
off_t lseek64(int fd, off_t offset, int whence);
ssize_t read(int fd, void *buf, size_t count);
std::vector<read_task> read_async(int fd, size_t count);
std::size_t read_wait(void *ptr, std::vector<read_task> &tasks,
                       std::string filename);
std::vector<write_task *> write_async(int fd, const void *buf, size_t count);
size_t write_wait(std::vector<write_task *> tasks);
ssize_t write(int fd, const void *buf, size_t count);
} // namespace dtio

#endif // DTIO_MAIN_POSIX_H
