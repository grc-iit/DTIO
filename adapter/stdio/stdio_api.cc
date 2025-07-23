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

#include <cstdio>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
// #include "hermes_shm/util/logging.h"
// #include <filesystem>

// #include "hermes_types.h"
#include "interceptor.h"
#include <dtio/drivers/stdio.h>
#include <dtio/singleton.h>
// #include <dtio/drivers/mpi.h>

#include "stdio_api.h"
// #include "posix_fs_api.h"
// #include "filesystem/filesystem.h"

// using hermes::adapter::fs::AdapterStat;
// using hermes::adapter::fs::IoStatus;
// using hermes::adapter::fs::File;
// using hermes::adapter::fs::SeekMode;

// namespace hapi = hermes::api;
namespace stdfs = std::filesystem;

std::shared_ptr<dtio::stdio::StdioApi> dtio::stdio::StdioApi::instance
    = nullptr;

extern "C"
{

  static __attribute__ ((constructor (101))) void
  init_posix (void)
  {
    HERMES_STDIO_API;
    // HERMES_POSIX_FS;
    // TRANSPARENT_HERMES;
  } /**/

  /**
   * MPI
   */
  int
  HERMES_DECL (MPI_Init) (int *argc, char ***argv)
  {
    DTIO_LOG_DEBUG_RANKLESS ("Intercepted " << __func__);
    auto real_api = HERMES_STDIO_API;
    // You shouldn't ever be calling the real version

    std::string execname = std::string (*argv[0]);
    DTIO_LOG_DEBUG_RANKLESS ("Now intercepting executable: " << execname);
    real_api->add_to_whitelist (execname);
    return dtio::MPI_Init (argc, argv);
  }
  // void HERMES_DECL(MPI_Finalize)() {
  //   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  //   // You shouldn't ever be calling the real version

  //   return dtio::MPI_Finalize();
  // }

  /**
   * STDIO
   */
  FILE *
  HERMES_DECL (fopen) (const char *filename, const char *mode)
  {
    DTIO_LOG_DEBUG_RANKLESS ("Intercepted " << __func__);
    auto real_api = HERMES_STDIO_API;

    std::string caller_name = "";
    if (real_api->check_path (filename))
      {
        FILE *retfp = dtio::stdio::fopen (filename, mode);
        real_api->whitelist_fp (retfp);
        return retfp;
      }
    else if (ConfigManager::get_instance ()->NEVER_TRACE)
      {
        return real_api->fopen (filename, mode);
      }
    if (boost::stacktrace::stacktrace ().size () > 1)
      {
        caller_name = boost::stacktrace::detail::location_from_symbol (
                          boost::stacktrace::stacktrace ()[1].address ())
                          .name ();
      }

    if (real_api->interceptp (caller_name))
      {
        FILE *retfp = dtio::stdio::fopen (filename, mode);
        real_api->whitelist_fp (retfp);
        return retfp;
      }
    else
      {
        return real_api->fopen (filename, mode);
      }
  }

  char *
  HERMES_DECL (fgets) (char *ptr, int count, FILE *stream)
  {
    DTIO_LOG_DEBUG_RANKLESS ("Intercepted " << __func__);
    auto real_api = HERMES_STDIO_API;
    std::string caller_name = "";

    if (real_api->fp_whitelistedp (stream))
      {
        // if (ConfigManager::get_instance()->ASYNC) {
        //   return dtio::posix::read_async(fd, buf, count);
        // }
        // else {
        dtio::stdio::fread (ptr, 1, count, stream, true);
        return ptr;
        // }
      }
    else
      {
        return real_api->fgets (ptr, count, stream);
      }

    if (boost::stacktrace::stacktrace ().size () > 1)
      {
        caller_name = boost::stacktrace::detail::location_from_symbol (
                          boost::stacktrace::stacktrace ()[1].address ())
                          .name ();
      }
    auto test = real_api->interceptp (caller_name);
    if (real_api->interceptp (caller_name))
      {
        // if (ConfigManager::get_instance()->ASYNC) {
        //   return dtio::posix::read_async(fd, buf, count);
        // }
        // else {
        dtio::stdio::fread (ptr, 1, count, stream, true);

        return ptr;
        // }
      }
    else
      {
        return real_api->fgets (ptr, count, stream);
      }
  }

  size_t
  HERMES_DECL (fread) (void *ptr, size_t size, size_t count, FILE *stream)
  {
    DTIO_LOG_DEBUG_RANKLESS ("Intercepted " << __func__);
    auto real_api = HERMES_STDIO_API;
    std::string caller_name = "";

    if (real_api->fp_whitelistedp (stream))
      {
        // if (ConfigManager::get_instance()->ASYNC) {
        //   return dtio::posix::read_async(fd, buf, count);
        // }
        // else {
        return dtio::stdio::fread (ptr, size, count, stream);
        // }
      }
    else
      {
        return real_api->fread (ptr, size, count, stream);
      }

    if (boost::stacktrace::stacktrace ().size () > 1)
      {
        caller_name = boost::stacktrace::detail::location_from_symbol (
                          boost::stacktrace::stacktrace ()[1].address ())
                          .name ();
      }
    auto test = real_api->interceptp (caller_name);
    if (real_api->interceptp (caller_name))
      {
        // if (ConfigManager::get_instance()->ASYNC) {
        //   return dtio::posix::read_async(fd, buf, count);
        // }
        // else {
        return dtio::stdio::fread (ptr, size, count, stream);
        // }
      }
    else
      {
        return real_api->fread (ptr, size, count, stream);
      }
  }

  size_t
  HERMES_DECL (fwrite) (const void *ptr, size_t size, size_t count,
                        FILE *stream)
  {
    DTIO_LOG_DEBUG_RANKLESS ("Intercepted " << __func__);
    auto real_api = HERMES_STDIO_API;
    std::string caller_name = "";

    if (real_api->fp_whitelistedp (stream))
      {
        if (ConfigManager::get_instance ()->ASYNC)
          {
            return dtio::stdio::fwrite (
                ptr, size, count, stream); // TODO eventually, write_async
          }
        else
          {
            return dtio::stdio::fwrite (ptr, size, count, stream);
          }
      }
    else
      {
        return real_api->fwrite (ptr, size, count, stream);
      }

    if (boost::stacktrace::stacktrace ().size () > 1)
      {
        caller_name = boost::stacktrace::detail::location_from_symbol (
                          boost::stacktrace::stacktrace ()[1].address ())
                          .name ();
      }
    if (real_api->interceptp (caller_name))
      {
        if (ConfigManager::get_instance ()->ASYNC)
          {
            return dtio::stdio::fwrite (
                ptr, size, count, stream); // TODO eventually, write_async
          }
        else
          {
            return dtio::stdio::fwrite (ptr, size, count, stream);
          }
      }
    else
      {
        return real_api->fwrite (ptr, size, count, stream);
      }
  }

  int
  HERMES_DECL (fseek) (FILE *stream, long int offset, int origin)
  {
    DTIO_LOG_DEBUG_RANKLESS ("Intercepted " << __func__);
    auto real_api = HERMES_STDIO_API;
    std::string caller_name = "";

    if (real_api->fp_whitelistedp (stream))
      {
        return dtio::stdio::fseek (stream, offset, origin);
      }
    else
      {
        return real_api->fseek (stream, offset, origin);
      }

    if (boost::stacktrace::stacktrace ().size () > 1)
      {
        caller_name = boost::stacktrace::detail::location_from_symbol (
                          boost::stacktrace::stacktrace ()[1].address ())
                          .name ();
      }
    if (real_api->interceptp (caller_name))
      {
        return dtio::stdio::fseek (stream, offset, origin);
      }
    else
      {
        return real_api->fseek (stream, offset, origin);
      }
  }

  // int HERMES_DECL(stat)(const char *pathname, struct stat *buf) {
  //   DTIO_LOG_DEBUG_RANKLESS("Intercepted " << __func__);
  //   auto real_api = HERMES_STDIO_API;
  //   std::string caller_name = "";

  //   if (real_api->check_path(pathname)) {
  //     return dtio::posix::mystat(pathname, buf);
  //   }
  //   else if (ConfigManager::get_instance()->NEVER_TRACE) {
  //     return real_api->stat(pathname, buf);
  //   }

  //   if (boost::stacktrace::stacktrace().size() > 1) {
  //     caller_name =
  //     boost::stacktrace::detail::location_from_symbol(boost::stacktrace::stacktrace()[1].address()).name();
  //   }
  //   if (real_api->interceptp(caller_name)) {
  //     return dtio::posix::mystat(pathname, buf);
  //   }
  //   else {
  //     return real_api->stat(pathname, buf);
  //   }
  // }

  int
  HERMES_DECL (fclose) (FILE *stream)
  {
    DTIO_LOG_DEBUG_RANKLESS ("Intercepted " << __func__);
    auto real_api = HERMES_STDIO_API;
    std::string caller_name = "";
    if (real_api->fp_whitelistedp (stream))
      {
        return dtio::stdio::fclose (stream);
      }
    else
      {
        return real_api->fclose (stream);
      }
    if (boost::stacktrace::stacktrace ().size () > 1)
      {
        caller_name = boost::stacktrace::detail::location_from_symbol (
                          boost::stacktrace::stacktrace ()[1].address ())
                          .name ();
      }
    if (real_api->interceptp (caller_name))
      {
        return dtio::stdio::fclose (stream);
      }
    else
      {
        return real_api->fclose (stream);
      }
  }

} // extern C
