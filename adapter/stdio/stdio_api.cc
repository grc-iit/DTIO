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
bool stdio_intercepted = true;

#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <cstdio>
// #include "hermes_shm/util/logging.h"
// #include <filesystem>

// #include "hermes_types.h"
// #include <dtio/drivers/stdio.h>  // Not available, using stdio_api.h instead

#include "dtio/client_metadata_manager.h"
#include "dtio/config_manager.h"
#include "dtio/dtio_enumerations.h"
#include "interceptor.h"
#include "stdio_api.h"
// #include "posix_fs_api.h"
// #include "filesystem/filesystem.h"

// using hermes::adapter::fs::AdapterStat;
// using hermes::adapter::fs::IoStatus;
// using hermes::adapter::fs::File;
// using hermes::adapter::fs::SeekMode;

// namespace hapi = hermes::api;
namespace stdfs = std::filesystem;

extern "C" {

static __attribute__((constructor(101))) void init_posix(void) {}

/**
 * MPI
 */
int HERMES_DECL(MPI_Init)(int *argc, char ***argv) {
  return 0;  // Success
}

/**
 * STDIO
 */
FILE *HERMES_DECL(fopen)(const char *filename, const char *mode) {
  // Call real fopen first
  FILE *real_fp = HERMES_STDIO_API->fopen(filename, mode);
  if (!real_fp) {
    return nullptr;
  }

  // Convert to absolute path
  std::string abs_path = std::filesystem::absolute(filename).string();

  // Check if should intercept
  auto *config = DTIO_CONF;
  if (!config->ShouldIntercept(abs_path)) {
    return real_fp;
  }

  // Register with metadata manager
  auto *client_meta = DTIO_CLIENT_META;
  int flags = 0;  // Convert mode to flags if needed
  if (strchr(mode, 'w')) flags |= O_WRONLY;
  if (strchr(mode, 'r')) flags |= O_RDONLY;
  if (strchr(mode, '+')) flags |= O_RDWR;
  if (strchr(mode, 'a')) flags |= O_APPEND;

  client_meta->RegisterStdioFp(real_fp, abs_path, flags);

  return real_fp;
}

char *HERMES_DECL(fgets)(char *ptr, int count, FILE *stream) {
  return nullptr;  // Not implemented
}

size_t HERMES_DECL(fread)(void *ptr, size_t size, size_t count, FILE *stream) {
  return 0;  // Not implemented
}

size_t HERMES_DECL(fwrite)(const void *ptr, size_t size, size_t count,
                           FILE *stream) {
  // Check if file pointer is registered with DTIO
  auto *client_meta = DTIO_CLIENT_META;
  if (!client_meta->IsStdioFpRegistered(stream)) {
    // Not intercepted, call real API
    return HERMES_STDIO_API->fwrite(ptr, size, count, stream);
  }

  // Get file info and submit to DTIO runtime
  auto *file_info = client_meta->GetStdioFileInfo(stream);
  if (file_info) {
    auto *config = DTIO_CONF;

    size_t total_size = size * count;

    // Allocate buffer in shared memory
    hipc::FullPtr<char> shm_buf =
        CHI_CLIENT->AllocateBuffer(HSHM_MCTX, total_size);
    memcpy(shm_buf.ptr_, ptr, total_size);

    // Submit write task with filename as chi::string
    config->dtio_mod_.Write(
        HSHM_MCTX, shm_buf.shm_, total_size, file_info->current_offset,
        chi::string(file_info->absolute_path), dtio::IoClientType::kStdio);

    // Update offset
    client_meta->UpdateStdioOffset(stream,
                                   file_info->current_offset + total_size);

    // Free shared memory buffer
    CHI_CLIENT->FreeBuffer(HSHM_MCTX, shm_buf);

    return count;
  }

  // Fallback to real API
  return HERMES_STDIO_API->fwrite(ptr, size, count, stream);
}

int HERMES_DECL(fseek)(FILE *stream, long int offset, int origin) {
  return -1;  // Not implemented
}

int HERMES_DECL(fclose)(FILE *stream) {
  // Remove from metadata manager if registered
  auto *client_meta = DTIO_CLIENT_META;
  if (client_meta->IsStdioFpRegistered(stream)) {
    client_meta->UnregisterStdioFp(stream);
  }

  // Call real fclose
  return HERMES_STDIO_API->fclose(stream);
}

}  // extern C
