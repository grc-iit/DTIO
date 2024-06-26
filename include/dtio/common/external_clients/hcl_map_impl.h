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
#ifndef DTIO_MAIN_HCLMAPIMPL_H
#define DTIO_MAIN_HCLMAPIMPL_H

#include "dtio/common/enumerations.h"
#include <city.h>
#include <cstring>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <dtio/common/config_manager.h>
#include <dtio/common/data_structures.h>
#include <hcl.h>

class HCLMapImpl : public distributed_hashmap
{

private:
  service map_service;
  hcl::unordered_map<struct HCLKeyType, DTIOCharStruct> *hcl_client;
  hcl::unordered_map<struct HCLKeyType, file_stat> *filestat_map;
  hcl::unordered_map<struct HCLKeyType, chunk_meta> *chunkmeta_map;
  // hcl::unordered_map<struct HCLKeyType, std::string, std::hash<HCLKeyType>,
  //                    CharAllocator, MappedUnitString> *hcl_client;
  size_t num_servers;
  std::string hclmapname;
  std::string get_server (std::string key);

public:
  // Constructor
  HCLMapImpl (service service, std::string mapname, int my_server,
	      int hcl_servers) : distributed_hashmap (service) {
    if (service == HCLCLIENT)
      {
	HCL_CONF->IS_SERVER = true;
      }
    else if (service == WORKER || service == TASK_SCHEDULER || service == LIB)
      {
	HCL_CONF->IS_SERVER = false;
      }
    else
      {
	std::cout << "I'm uncertain why this happens " << service << std::endl;
      }

    hclmapname = mapname;
    num_servers = hcl_servers;
    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = true; // (service != LIB)
    HCL_CONF->SERVER_LIST_PATH
      = ConfigManager::get_instance ()->HCL_SERVER_LIST_PATH;

    if ((service == WORKER || service == TASK_SCHEDULER) && mapname == "dataspace")
      {
	MPI_Barrier (
		     ConfigManager::get_instance ()
		     ->DATASPACE_COMM); // Wait for clients to initialize maps
      }
    else if ((service == WORKER || service == TASK_SCHEDULER) && mapname == "metadata")
      {
	MPI_Barrier (
		     ConfigManager::get_instance ()
		     ->METADATA_COMM); // Wait for clients to initialize maps
      }

    if (mapname == "dataspace") {
      hcl_client = new hcl::unordered_map<HCLKeyType, DTIOCharStruct> (mapname);
    }
    else {
      hcl_client = new hcl::unordered_map<HCLKeyType, DTIOCharStruct> (mapname);
      filestat_map = new hcl::unordered_map<HCLKeyType, file_stat> (mapname + "+fs");
      chunkmeta_map = new hcl::unordered_map<HCLKeyType, chunk_meta> (mapname + "+chunkmeta");
    }

    // hcl_client = new hcl::unordered_map<HCLKeyType, std::string,
    // 					std::hash<HCLKeyType>, CharAllocator,
    // 					MappedUnitString> (mapname);
    if (service == HCLCLIENT && mapname == "dataspace")
      {
	MPI_Barrier (
		     ConfigManager::get_instance ()
		     ->DATASPACE_COMM); // Tell the workers we've initialized maps
      }
    else if (service == HCLCLIENT && mapname == "metadata") {
	MPI_Barrier (
		     ConfigManager::get_instance ()
		     ->METADATA_COMM); // Tell the workers we've initialized maps

    }
  }

  int run();

  size_t
  get_servers () override
  {
    return num_servers;
  }

  int put (const table &name, std::string key, const std::string &value,
           std::string group_key) override;
  int put (const table &name, std::string key, const char *value, size_t size,
           std::string group_key) override;
  int put (const table &name, std::string key, chunk_meta *data,
	   std::string group_key) override;
  int put (const table &name, std::string key, file_stat *fs,
	   std::string group_key) override;

  std::string get (const table &name, std::string key,
                   std::string group_key);
  void get (const table &name, std::string key,
	    std::string group_key, char *result) override;
  bool get (const table &name, std::string key,
	    std::string group_key, chunk_meta *result) override;
  bool get (const table &name, std::string key,
	    std::string group_key, file_stat *result) override;
  std::string remove (const table &name, std::string key,
                      std::string group_key) override;
  bool exists (const table &name, std::string key,
               std::string group_key) override;
  bool purge () override;

  size_t counter_init (const table &name, std::string key,
                       std::string group_key) override;

  size_t counter_inc (const table &name, std::string key,
                      std::string group_key) override;

  virtual ~HCLMapImpl () { delete hcl_client; }
};

#endif // DTIO_MAIN_HCLMAPimpl_H
