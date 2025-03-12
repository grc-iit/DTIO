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
  // hcl::unordered_map<struct HCLKeyType, std::string, std::hash<HCLKeyType>,
  //                    CharAllocator, MappedUnitString> *hcl_client;
  size_t num_servers;
  std::string hclmapname;
  std::string get_server (std::string key, const table &tab);

  hcl::unordered_map<struct HCLKeyType, std::string, std::hash<HCLKeyType>, CharAllocator, MappedUnitString> *hcl_string_client;
public:
  hcl::unordered_map<struct HCLKeyType, DTIOCharStruct> *hcl_client;
  hcl::unordered_map<struct HCLKeyType, file_stat> *filestat_map; // I might need to move these to a separate process
  hcl::unordered_map<struct HCLKeyType, chunk_meta> *chunkmeta_map; // ^
  hcl::unordered_map<struct HCLKeyType, file_meta> *filemeta_map; // ^
  // std::shared_ptr<hcl::HCL> hcl;

  // Constructor
  HCLMapImpl (service service, std::string mapname, int my_server,
	      int hcl_servers, std::shared_ptr<hcl::HCL> &hcl_init) : distributed_hashmap (service) {
    try {
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
	DTIO_LOG_INFO("I'm uncertain why this happens " << service);
      }

    hclmapname = mapname;
    num_servers = 1; //hcl_servers;
    // if (mapname == "dataspace") {
    //   HCL_CONF->MY_SERVER = 0;
    // }
    // else if (mapname == "metadata") {
    //   HCL_CONF->MY_SERVER = 1;
    // }
    // else if (mapname == "metadata+fs") {
    //   HCL_CONF->MY_SERVER = 2;
    // }
    // else if (mapname == "metadata+chunkmeta") {
    //   HCL_CONF->MY_SERVER = 3;
    // }
    HCL_CONF->MY_SERVER = 0; //mapname == "dataspace" ? 0 : 1;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = HCL_CONF->IS_SERVER; // true; // (service != LIB)
    HCL_CONF->SERVER_LIST_PATH
      = ConfigManager::get_instance ()->HCL_SERVER_LIST_PATH;

    if ((service == WORKER || service == TASK_SCHEDULER) && mapname == "dataspace")
      {
	MPI_Barrier (
		     ConfigManager::get_instance ()
		     ->DATASPACE_COMM); // Wait for clients to initialize maps
      }
    else if ((service == WORKER || service == TASK_SCHEDULER) && (mapname == "metadata" || mapname == "metadata+fs" || mapname == "metadata+filemeta"))
      {
	MPI_Barrier (
		     ConfigManager::get_instance ()
		     ->METADATA_COMM); // Wait for clients to initialize maps
      }

    // if (mapname == "dataspace") {
    //   // HCL_CONF->RPC_PORT = 9001;
    // }
    // if (mapname == "metadata") {
    //   // HCL_CONF->RPC_PORT = 9002;
    //   hcl = hcl::HCL::GetInstance(true, 9002);
    //   hcl->ReConfigure(9002);
    // }
    if (mapname == "dataspace") {
      if (hcl_init == nullptr) {
	hcl_init = hcl::HCL::GetInstance(true, 9000);
      }
      else {
	hcl_init->ReConfigure(9000);
      }
    }
    else if (mapname == "metadata") {
      if (hcl_init == nullptr) {
	hcl_init = hcl::HCL::GetInstance(true, 9100);
      }
      else {
	hcl_init->ReConfigure(9100);
      }
    }
    else if (mapname == "metadata+fs") {
      if (hcl_init == nullptr) {
	hcl_init = hcl::HCL::GetInstance(true, 9200);
      }
      else {
	hcl_init->ReConfigure(9200);
      }
    }
    else if (mapname == "metadata+filemeta") {
      if (hcl_init == nullptr) {
	hcl_init = hcl::HCL::GetInstance(true, 9300);
      }
      else {
	hcl_init->ReConfigure(9300);
      }

    }
    if (mapname == "dataspace") {
#if(STACK_ALLOCATION)
      hcl_client = new hcl::unordered_map<HCLKeyType, DTIOCharStruct> (mapname);
#else
      hcl_string_client = new hcl::unordered_map<HCLKeyType, std::string, std::hash<HCLKeyType>, CharAllocator, MappedUnitString>(mapname);
#endif
    }
    else if (mapname == "metadata") {
#if(STACK_ALLOCATION)
      hcl_client = new hcl::unordered_map<HCLKeyType, DTIOCharStruct> (mapname);
#else
      hcl_string_client = new hcl::unordered_map<HCLKeyType, std::string, std::hash<HCLKeyType>, CharAllocator, MappedUnitString>(mapname);
#endif
    }
    else if (mapname == "metadata+fs") {
      filestat_map = new hcl::unordered_map<HCLKeyType, file_stat> (mapname + "+fs");
    }
    else if (mapname == "metadata+chunkmeta") {
      chunkmeta_map = new hcl::unordered_map<HCLKeyType, chunk_meta> (mapname + "+chunkmeta");
    }
    else if (mapname == "metadata+filemeta") {
      filemeta_map = new hcl::unordered_map<HCLKeyType, file_meta> (mapname + "+filemeta");
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
    else if (service == HCLCLIENT && (mapname == "metadata" || mapname == "metadata+fs" || mapname == "metadata+filemeta")) {
      // Three barriers here is a hack, one for each map type so that we can just use one communicator for everybody.
      MPI_Barrier (
		   ConfigManager::get_instance ()
		   ->METADATA_COMM); // Tell the workers we've initialized maps
      MPI_Barrier (
		   ConfigManager::get_instance ()
		   ->METADATA_COMM); // Tell the workers we've initialized maps
      MPI_Barrier (
		   ConfigManager::get_instance ()
		   ->METADATA_COMM); // Tell the workers we've initialized maps
    }
    } catch (std::logic_error *e) { std::cerr << "Error creating map on service " << service << ", " << e->what() << std::endl; exit(EXIT_FAILURE); }
    
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
  int put (const table &name, std::string key, file_meta *fm,
	   std::string group_key) override;

  std::string get (const table &name, std::string key,
                   std::string group_key);
  void get (const table &name, std::string key,
	    std::string group_key, char *result) override;
  void get (const table &name, std::string key,
	    std::string group_key, char *result, int max_size) override;
  bool get (const table &name, std::string key,
	    std::string group_key, chunk_meta *result) override;
  bool get (const table &name, std::string key,
	    std::string group_key, file_stat *result) override;
  bool get (const table &name, std::string key, std::string group_key, file_meta *result) override;


  std::string remove (const table &name, std::string key,
                      std::string group_key) override;
  bool exists (const table &name, std::string key,
               std::string group_key) override;
  bool purge () override;

  size_t counter_init (const table &name, std::string key,
                       std::string group_key) override;

  size_t counter_inc (const table &name, std::string key,
                      std::string group_key) override;

  virtual ~HCLMapImpl () {
    // hcl->Finalize();
    // hcl.reset();
  }
};

#endif // DTIO_MAIN_HCLMAPimpl_H
