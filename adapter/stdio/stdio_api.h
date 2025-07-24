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

#ifndef HERMES_ADAPTER_STDIO_H
#define HERMES_ADAPTER_STDIO_H
#include "real_api.h"
// #include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <boost/stacktrace.hpp>
#include <iostream>
#include <set>
#include <string>
// #include <unistd.h>
#include <cstdio>

#ifndef O_TMPFILE
#define O_TMPFILE 0
#endif

#ifndef _STAT_VER
#define _STAT_VER 0
#endif

extern "C" {
typedef FILE *(*fopen_t)(const char *filename, const char *mode);
typedef size_t (*fread_t)(void *ptr, size_t size, size_t count, FILE *stream);
typedef char *(*fgets_t)(char *ptr, int count, FILE *stream);
typedef size_t (*fwrite_t)(const void *ptr, size_t size, size_t count,
                           FILE *stream);
typedef int (*fseek_t)(FILE *stream, long int offset, int origin);
typedef int (*fclose_t)(FILE *stream);
}

namespace dtio::stdio {

/** Used for compatability with older kernel versions */
static int fxstat_to_fstat(int fd, struct stat *stbuf);

/** Pointers to the real stdio API */
class StdioApi : public dtio::adapter::RealApi {
 private:
 public:
  std::shared_ptr<std::set<std::string> > interception_whitelist = nullptr;
  std::shared_ptr<std::set<FILE *> > fp_interception_whitelist = nullptr;

  /** fopen */
  fopen_t fopen = nullptr;
  /** fread */
  fread_t fread = nullptr;
  /** fgets */
  fgets_t fgets = nullptr;
  /** fwrite */
  fwrite_t fwrite = nullptr;
  /** fseek */
  fseek_t fseek = nullptr;
  /** fclose */
  fclose_t fclose = nullptr;

  StdioApi() : dtio::adapter::RealApi("open", "stdio_intercepted") {
    interception_whitelist = std::make_shared<std::set<std::string> >();
    fp_interception_whitelist = std::make_shared<std::set<FILE *> >();

    fopen = (fopen_t)dlsym(real_lib_, "fopen");
    REQUIRE_API(fopen)
    fread = (fread_t)dlsym(real_lib_, "fread");
    REQUIRE_API(fread)
    fgets = (fgets_t)dlsym(real_lib_, "fgets");
    REQUIRE_API(fgets)
    fwrite = (fwrite_t)dlsym(real_lib_, "fwrite");
    REQUIRE_API(fwrite)
    fseek = (fseek_t)dlsym(real_lib_, "fseek");
    REQUIRE_API(fseek)

    fclose = (fclose_t)dlsym(real_lib_, "fclose");
    REQUIRE_API(fclose)
  }
  StdioApi(const StdioApi &) = default;
  StdioApi(StdioApi &&) = default;
  StdioApi &operator=(const StdioApi &) = default;
  StdioApi &operator=(StdioApi &&) = default;
};
}  // namespace dtio::stdio

// Singleton macros
#include "hermes_shm/util/singleton.h"

#define HERMES_STDIO_API hshm::Singleton<dtio::stdio::StdioApi>::GetInstance()
#define HERMES_STDIO_API_T dtio::stdio::StdioApi *

#endif  // HERMES_ADAPTER_STDIO
