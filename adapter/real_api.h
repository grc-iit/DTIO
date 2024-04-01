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

#ifndef HERMES_ADAPTER_API_H
#define HERMES_ADAPTER_API_H

#include <dlfcn.h>
#include <link.h>
#include <dtio/common/logger.h>

#define REQUIRE_API(api_name)                                                 \
  if (!(api_name))                                                            \
    {                                                                         \
      DTIO_LOG_ERROR ("HERMES Adapter failed to map symbol: {}", #api_name);   \
    }

namespace  dtio::adapter
{

struct RealApiIter
{
  const char *symbol_name_;
  const char *is_intercepted_;
  const char *lib_path_;

  RealApiIter (const char *symbol_name, const char *is_intercepted)
      : symbol_name_ (symbol_name), is_intercepted_ (is_intercepted),
        lib_path_ (nullptr)
  {
  }
};

struct RealApi
{
  void *real_lib_;
  bool is_intercepted_;

  static int
  callback (struct dl_phdr_info *info, size_t size, void *data)
  {
    auto iter = (RealApiIter *)data;
    auto lib = dlopen (info->dlpi_name, RTLD_GLOBAL | RTLD_NOW);
    auto exists = dlsym (lib, iter->symbol_name_);
    void *is_intercepted = (void *)dlsym (lib, iter->is_intercepted_);
    if (!is_intercepted && exists && !iter->lib_path_)
      {
        iter->lib_path_ = info->dlpi_name;
      }
    return 0;
  }

  RealApi (const char *symbol_name, const char *is_intercepted)
  {
    RealApiIter iter (symbol_name, is_intercepted);
    dl_iterate_phdr (callback, (void *)&iter);
    if (iter.lib_path_)
      {
        real_lib_ = dlopen (iter.lib_path_, RTLD_GLOBAL | RTLD_NOW);
      }
    void *is_intercepted_ptr = (void *)dlsym (RTLD_DEFAULT, is_intercepted);
    is_intercepted_ = is_intercepted_ptr != nullptr;
    /* if (is_intercepted_) {
      real_lib_ = RTLD_NEXT;
    } else {
      real_lib_ = RTLD_DEFAULT;
    }*/
  }
};

} // namespace dtio::adapter

#endif // HERMES_ADAPTER_API_H
