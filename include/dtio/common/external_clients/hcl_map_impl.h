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
/*
 * Created by hariharan on 2/16/18 (around).
 * Updated by kbateman and nrajesh (nrj5k) since
 */
#ifndef DTIO_MAIN_HCLMAPIMPL_H
#define DTIO_MAIN_HCLMAPIMPL_H

#include <city.h>
#include <cstring>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <dtio/common/config_manager.h>
#include <dtio/common/data_structures.h>
#include <hcl.h>

class HCLMapImpl : public distributed_hashmap
{

private:
  hcl::unordered_map<struct HCLKeyType, std::string, std::hash<HCLKeyType>, CharAllocator, MappedUnitString> *hcl_client;
  size_t num_servers;
  std::string get_server (std::string key);

public:
  // Constructor
  HCLMapImpl(service service, bool is_server, int my_server, int num_servers, bool server_on_node=false)
      : distributed_hashmap(service) {
    HCL_CONF->IS_SERVER = is_server;
    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = server_on_node;
    // HCL_CONF->SERVER_LIST_PATH = ConfigManager::get_instance()->HCL_SERVER_LIST;

    MPI_Barrier(ConfigManager::get_instance ()
		->DATASPACE_COMM); // Ideally, we'd have a communicator
                                   // for maps specifically
    hcl_client = new hcl::unordered_map<HCLKeyType, std::string, std::hash<HCLKeyType>, CharAllocator, MappedUnitString>(); // Map needs to be spawned on multiple servers, clients and workers at the same time. This will be client-side.
    num_servers = HCL_CONF->NUM_SERVERS;
  }
  size_t
  get_servers () override
  {
    return num_servers;
  }

  int put (const table &name, std::string key, const std::string &value,
           std::string group_key) override;
  std::string get (const table &name, std::string key,
                   std::string group_key) override;
  std::string remove (const table &name, std::string key,
                      std::string group_key) override;
  bool exists (const table &name, std::string key,
               std::string group_key) override;
  bool purge () override;

  size_t counter_init (const table &name, std::string key,
                       std::string group_key) override;

  size_t counter_inc (const table &name, std::string key,
                      std::string group_key) override;


  virtual ~HCLMapImpl () {}
};

#endif // DTIO_MAIN_HCLMAPimpl_H
