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
#ifndef DTIO_MAIN_IOWARPMAPIMPL_H
#define DTIO_MAIN_IOWARPMAPIMPL_H

#include "dtio/enumerations.h"
#include <city.h>
#include <cstring>
#include <dtio/client_interface/distributed_hashmap.h>
#include <dtio/config_manager.h>
#include <dtio/data_structures.h>
#include <dtiomod/dtiomod_client.h>

class IOWARPMapImpl : public distributed_hashmap
{
private:
  service map_service;
  // iowarp::unordered_map<struct IOWARPKeyType, std::string,
  // std::hash<IOWARPKeyType>,
  //                    CharAllocator, MappedUnitString> *iowarp_client;
  std::string iowarpmapname;
  std::string get_server (std::string key, const table &tab);

  // iowarp::unordered_map<struct IOWARPKeyType, std::string,
  // std::hash<IOWARPKeyType>, CharAllocator, MappedUnitString>
  // *iowarp_string_client;
public:
  // iowarp::unordered_map<struct IOWARPKeyType, DTIOCharStruct>
  // *iowarp_client; iowarp::unordered_map<struct IOWARPKeyType, file_stat>
  // *filestat_map; // I might need to move these to a separate process
  // iowarp::unordered_map<struct IOWARPKeyType, chunk_meta> *chunkmeta_map; //
  // ^ iowarp::unordered_map<struct IOWARPKeyType, file_meta> *filemeta_map; //
  // ^
  // // std::shared_ptr<iowarp::IOWARP> iowarp;

  // Constructor
  IOWARPMapImpl (service service, std::string mapname)
      : distributed_hashmap (service)
  {
    iowarpmapname = mapname;

    //     if (mapname == "dataspace") {
    // #if(STACK_ALLOCATION)
    //       iowarp_client = new iowarp::unordered_map<IOWARPKeyType,
    //       DTIOCharStruct> (mapname);
    // #else
    //       iowarp_string_client = new iowarp::unordered_map<IOWARPKeyType,
    //       std::string, std::hash<IOWARPKeyType>, CharAllocator,
    //       MappedUnitString>(mapname);
    // #endif
    //     }
    //     else if (mapname == "metadata") {
    // #if(STACK_ALLOCATION)
    //       iowarp_client = new iowarp::unordered_map<IOWARPKeyType,
    //       DTIOCharStruct> (mapname);
    // #else
    //       iowarp_string_client = new iowarp::unordered_map<IOWARPKeyType,
    //       std::string, std::hash<IOWARPKeyType>, CharAllocator,
    //       MappedUnitString>(mapname);
    // #endif
    //     }
    //     else if (mapname == "metadata+fs") {
    //       filestat_map = new iowarp::unordered_map<IOWARPKeyType, file_stat>
    //       (mapname + "+fs");
    //     }
    //     else if (mapname == "metadata+chunkmeta") {
    //       chunkmeta_map = new iowarp::unordered_map<IOWARPKeyType,
    //       chunk_meta> (mapname + "+chunkmeta");
    //     }
    //     else if (mapname == "metadata+filemeta") {
    //       filemeta_map = new iowarp::unordered_map<IOWARPKeyType, file_meta>
    //       (mapname + "+filemeta");
    //     }
  }

  int run ();

  size_t
  get_servers () override
  {
    return 1; // num_servers;
  }

  int put (const table &name, std::string key, const std::string &value,
           std::string group_key) override;
  int put (const table &name, std::string key, const char *value, size_t size,
           std::string group_key) override;
  int put (const table &name, std::string key, chunk_meta *data,
           std::string group_key) override;
  int put (const table &name, std::string key, file_stat *fs,
           std::string group_key) override;
  int put (const table &name, std::string key, file_meta *fm,
           std::string group_key) override;

  std::string get (const table &name, std::string key, std::string group_key);
  void get (const table &name, std::string key, std::string group_key,
            char *result) override;
  void get (const table &name, std::string key, std::string group_key,
            char *result, int max_size) override;
  bool get (const table &name, std::string key, std::string group_key,
            chunk_meta *result) override;
  bool get (const table &name, std::string key, std::string group_key,
            file_stat *result) override;
  bool get (const table &name, std::string key, std::string group_key,
            file_meta *result) override;

  std::string remove (const table &name, std::string key,
                      std::string group_key) override;
  bool exists (const table &name, std::string key,
               std::string group_key) override;
  bool purge () override;

  size_t counter_init (const table &name, std::string key,
                       std::string group_key) override;

  size_t counter_inc (const table &name, std::string key,
                      std::string group_key) override;

  virtual ~IOWARPMapImpl () {}
};

#endif // DTIO_MAIN_IOWARPMAPimpl_H
