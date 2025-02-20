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
#include "dtio/common/constants.h"
#include <dtio/common/external_clients/hcl_map_impl.h>
#include <mpi.h>
// #include <boost/stacktrace.hpp>

int
HCLMapImpl::run()
{
  while (true); // Busy loop, the maps just support the data structures
}

int
HCLMapImpl::put (const table &name, std::string key, chunk_meta *data,
		 std::string group_key)
{
  if (name != table::CHUNK_DB) {
    DTIO_LOG_ERROR("Attempt to put chunk meta into unrelated database " << name);
  }
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (new_key);
  if (chunkmeta_map->Put (true_key, *data))
    {
      return 0;
    }
  else
    {
      DTIO_LOG_ERROR("put failed for key:" << new_key << "\n");
      return 1;
    }
}

int
HCLMapImpl::put (const table &name, std::string key, file_meta *fm,
		 std::string group_key)
{
  if (name != table::FILE_CHUNK_DB) {
    DTIO_LOG_ERROR("Attempt to put file stat into unrelated database " << name);
  }
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (new_key);
  // std::cout << "Put in " << hclmapname << "+filemeta at " << key << std::endl;
  // std::cout << fm->num_chunks << std::endl;
  if (fm->num_chunks > 0) {
    // std::cout << fm->chunks[fm->current_chunk_index - 1].actual_user_chunk.size << std::endl;
  }
  if (filemeta_map->Put (true_key, *fm))
    {
      return 0;
    }
  else
    {
      DTIO_LOG_ERROR("put failed for key:" << new_key << "\n");
      return 1;
    }
}

int
HCLMapImpl::put (const table &name, std::string key, file_stat *fs,
		 std::string group_key)
{
  if (name != table::FILE_DB) {
    DTIO_LOG_ERROR("Attempt to put file stat into unrelated database " << name);
  }
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (new_key);
  if (filestat_map->Put (true_key, *fs))
    {
      return 0;
    }
  else
    {
      DTIO_LOG_ERROR("put failed for key:" << new_key << "\n");
      return 1;
    }
}

int
HCLMapImpl::put (const table &name, std::string key, const std::string &value,
                 std::string group_key)
{
  // std::cout << boost::stacktrace::stacktrace();
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (new_key);
  DTIO_LOG_DEBUG("Put in " << hclmapname << " at " << new_key << " " << value << " size " << value.size());
  DTIO_LOG_INFO("Put in " << hclmapname << " at " << new_key << " size " << value.size());

#if(STACK_ALLOCATION)
  {
    DTIOCharStruct *true_val = new DTIOCharStruct(value);
    if (hcl_client->Put (true_key, *true_val))
      {
	delete(true_val);
	return 0;
      }
    else
      {
	delete(true_val);
	std::cerr << "put failed for key:" << new_key << "\n";
	return 1;
      }
  }
#else
  {
    auto true_val = std::string(value, value.length());
    if (hcl_string_client->Put (true_key, true_val))
      {
	return 0;
      }
    else
      {
	std::cerr << "put failed for key:" << new_key << "\n";
	return 1;
      }
  }
#endif
}

int
HCLMapImpl::put (const table &name, std::string key, const char *value, size_t size,
                 std::string group_key)
{
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (new_key);
  DTIO_LOG_DEBUG("Put in " << hclmapname << " at " << new_key << " " << value << " size " << size);
  DTIO_LOG_INFO("Put in " << hclmapname << " at " << new_key << " size " << size);

#if(STACK_ALLOCATION)
  {
    DTIOCharStruct *true_val = new DTIOCharStruct(value, size);
    if (hcl_client->Put (true_key, *true_val))
      {
	delete(true_val);
	return 0;
      }
    else
      {
	delete(true_val);
	std::cerr << "put failed for key:" << new_key << "\n";
	return 1;
      }
  }
#else
  {

    auto true_val = std::string(value, size);
    if (hcl_string_client->Put (true_key, true_val))
      {
	return 0;
      }
    else
      {
	std::cerr << "put failed for key:" << new_key << "\n";
	return 1;
      }   
  }
#endif
}


std::string
HCLMapImpl::get (const table &name, std::string key, std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  DTIO_LOG_DEBUG("Get in " << hclmapname << " at " << key);
  DTIO_LOG_INFO("Get in " << hclmapname << " at " << key);

  auto true_key = HCLKeyType (key);
#if(STACK_ALLOCATION)
  {
    auto const& retval = hcl_client->Get (true_key);
    if (retval.first)
      {
	DTIO_LOG_INFO("Get result size " << retval.second.length);
	return std::string(retval.second.value, retval.second.length);
      }
    else
      {
	return "";
      }
  }
#else
  {
    auto const& retval = hcl_string_client->Get (true_key);
    if (retval.first)
      {
	DTIO_LOG_INFO("Get result size " << retval.second.length);
	return retval.second;
      }
    else
      {
	return "";
      }    
  }
#endif
}

void
HCLMapImpl::get (const table &name, std::string key, std::string group_key, char *result, int max_size)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  DTIO_LOG_DEBUG("Get in " << hclmapname << " at " << key);
  DTIO_LOG_INFO("Get in " << hclmapname << " at " << key);

  auto true_key = HCLKeyType (key);
#if(STACK_ALLOCATION)
  {
    auto const& retval = hcl_client->Get (true_key);
    if (retval.first)
      {
	DTIO_LOG_INFO("Get result size " << retval.second.length);
	strncpy(result, retval.second.value, retval.second.length);
	// strncpy(result, retval.second.value, std::min(retval.second.length, max_size));
      }
  }
#else
  {
    auto const& retval = hcl_string_client->Get (true_key);
    if (retval.first)
      {
	DTIO_LOG_INFO("Get result size " << retval.second.length());
	strncpy(result, retval.second.data(), retval.second.length());
      }
  }
#endif
}

void
HCLMapImpl::get (const table &name, std::string key, std::string group_key, char *result)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  DTIO_LOG_DEBUG("Get in " << hclmapname << " at " << key);
  DTIO_LOG_INFO("Get in " << hclmapname << " at " << key);

  auto true_key = HCLKeyType (key);
#if(STACK_ALLOCATION)
  {
    auto const& retval = hcl_client->Get (true_key);
    if (retval.first)
      {
	DTIO_LOG_INFO("Get result size " << retval.second.length);
	strncpy(result, retval.second.value, retval.second.length);
      }
  }
#else
  {
    auto const& retval = hcl_string_client->Get (true_key);
    if (retval.first)
      {
	DTIO_LOG_INFO("Get result size " << retval.second.length());
	strncpy(result, retval.second.data(), retval.second.length());
      }
  }
#endif
}

bool
HCLMapImpl::get (const table &name, std::string key, std::string group_key, chunk_meta *result)
{
  if (name != table::CHUNK_DB) {
    DTIO_LOG_ERROR("Attempt to get chunk meta from unrelated database " << name);
  }

  key = std::to_string (name) + KEY_SEPARATOR + key;
  DTIO_LOG_DEBUG("Get in " << hclmapname << "+chunkmeta at " << key);
  auto true_key = HCLKeyType (key);
  auto const& retval = chunkmeta_map->Get (true_key);
  if (retval.first)
    {
      // DTIO_LOG_INFO("Get result size " << retval.second.length);
      *result = retval.second;
      // strncpy(result, retval.second.value, retval.second.length);
    }
  return retval.first;
}

bool
HCLMapImpl::get (const table &name, std::string key, std::string group_key, file_meta *result)
{
  if (name != table::FILE_CHUNK_DB) {
    DTIO_LOG_ERROR("Attempt to get file chunk meta from unrelated database " << name);
  }
  // std::cout << "Starting filemeta get" << std::endl;

  key = std::to_string (name) + KEY_SEPARATOR + key;
  DTIO_LOG_DEBUG("Get in " << hclmapname << "+filemeta at " << key);
  // std::cout << "Get in " << hclmapname << "+filemeta at " << key << std::endl;
  auto true_key = HCLKeyType (key);
  auto const& retval = filemeta_map->Get (true_key);
  // FIXME there's an error here
  // dtio_hclmanager: ../src/include/abti_mem_pool.h:123: ABTI_mem_pool_alloc: Assertion `num_headers_in_cur_bucket >= 1' failed.
  // std::cout << "Filemeta get complete" << std::endl;
  if (retval.first)
    {
      // std::cout << result->num_chunks << std::endl;
      if (result->num_chunks > 0) {
	// std::cout << result->chunks[result->current_chunk_index - 1].actual_user_chunk.size << std::endl;
      }
      // DTIO_LOG_INFO("Get result size " << retval.second.length);
      *result = retval.second;
      // strncpy(result, retval.second.value, retval.second.length);
    }
  return retval.first;
}

bool
HCLMapImpl::get (const table &name, std::string key, std::string group_key, file_stat *result)
{
  if (name != table::FILE_DB) {
    DTIO_LOG_ERROR("Attempt to get file stat from unrelated database " << name);
  }

  key = std::to_string (name) + KEY_SEPARATOR + key;
  DTIO_LOG_DEBUG("Get in " << hclmapname << "+fs at " << key);
  auto true_key = HCLKeyType (key);
  
  auto const& retval = filestat_map->Get (true_key);
  if (retval.first)
    {
      // DTIO_LOG_INFO("Get result size " << retval.second.length);
      *result = retval.second;
      // strncpy(result, retval.second.value, retval.second.length);
    }
  return retval.first;
}

std::string
HCLMapImpl::remove (const table &name, std::string key, std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (key);
  if (name == table::FILE_DB) {
    filestat_map->Erase (true_key);
  }
  else if (name == table::CHUNK_DB) {
    chunkmeta_map->Erase (true_key);
  }
  else {
#if(STACK_ALLOCATION)
    hcl_client->Erase (true_key);
#else
    hcl_string_client->Erase (true_key);
#endif
  }
  return key;
}

bool
HCLMapImpl::exists (const table &name, std::string key, std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (key);
  if (name == table::CHUNK_DB) {
    auto const& retval = chunkmeta_map->Get (true_key);
    return retval.first;
  }
  else if (name == table::FILE_DB) {
    auto const& retval = filestat_map->Get (true_key);
    return retval.first;
  }
  else {
#if(STACK_ALLOCATION)
    {
      auto const& retval = hcl_client->Get (true_key);
      return retval.first;
    }
#else
    {
      auto const& retval = hcl_string_client->Get (true_key);
      return retval.first;
    }
#endif
  }
}

bool
HCLMapImpl::purge ()
{
  // Not necessary, nothing calls it at the moment and memcached just does a
  // flush
}

size_t
HCLMapImpl::counter_init (const table &name, std::string key,
                          std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  if (group_key == "-1")
    {
      group_key = get_server (key, name);
    }
  return distributed_hashmap::counter_init (name, key, group_key);
}

size_t
HCLMapImpl::counter_inc (const table &name, std::string key,
                         std::string group_key)
{
  auto keyname = std::to_string (name) + KEY_SEPARATOR + key;

  if (group_key == "-1")
    {
      group_key = get_server (keyname, name);
    }
  if (this->exists (name, key, group_key))
    {
      char *val = (char *)malloc(MAX_IO_UNIT);
      get (name, key, group_key, val);
      size_t numeric_val = atoi (val);
      numeric_val++;
      put (name, key, std::to_string (numeric_val), group_key);
      delete(val);
      return numeric_val;
    }
  else
    {
      put (name, key, "0", group_key);
      return 0;
    }
}

std::string
HCLMapImpl::get_server (std::string key, const table &tab)
{
  // auto true_key = HCLKeyType (key);
  // std::hash<HCLKeyType> keyHash;
  // keyHash (true_key) % num_servers
  size_t server = 0; //hclmapname == "dataspace" ? 0 : 1;
  return std::to_string (server);
}
