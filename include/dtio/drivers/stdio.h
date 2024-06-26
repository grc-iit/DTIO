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
#ifndef DTIO_MAIN_STDIO_H
#define DTIO_MAIN_STDIO_H
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
FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);
int fseek(FILE *stream, long int offset, int origin);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
std::vector<task> fread_async(size_t size, size_t count, FILE *stream);
std::size_t fread_wait(void *ptr, std::vector<task> &tasks,
                       std::string filename);
std::vector<task *> fwrite_async(void *ptr, size_t size, size_t count,
				 FILE *stream);
size_t fwrite_wait(std::vector<task *> tasks);
size_t fwrite(void *ptr, size_t size, size_t count, FILE *stream);
} // namespace dtio

#endif // DTIO_MAIN_STDIO_H
