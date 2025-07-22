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
#include <dtio/common/external_clients/iowarp_map_impl.h>
#include <mpi.h>

int
IOWARPMapImpl::run()
{
  while (true); // Busy loop, the maps just support the data structures
}

int
IOWARPMapImpl::put (const table &name, std::string key, chunk_meta *data,
		 std::string group_key)
{
  if (name != table::CHUNK_DB) {
    DTIO_LOG_ERROR("Attempt to put chunk meta into unrelated database " << name);
  }
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;

  std::stringstream ss;
  cereal::BinaryOutputArchive ar(ss);
  ar(*data);

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(new_key.length());
  auto valstring = ss.str();
  size_t valsize = hshm::Unit<size_t>::Bytes(valstring.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  hipc::FullPtr<char> myval =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, valsize);
  char *keyptr = mykey.ptr_;
  char *valptr = myval.ptr_;
  strcpy(keyptr, new_key.c_str());
  memcpy(valptr, valstring.c_str(), valsize);
  client.MetaPut(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_,
	       keysize, myval.shm_, valsize);  

  return 0;
}

int
IOWARPMapImpl::put (const table &name, std::string key, file_meta *fm,
		 std::string group_key)
{
  if (name != table::FILE_CHUNK_DB) {
    DTIO_LOG_ERROR("Attempt to put file stat into unrelated database " << name);
  }
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;

  if (fm->num_chunks <= 0) {
    return 1;
  }

  std::stringstream ss;
  cereal::BinaryOutputArchive ar(ss);
  ar(*fm);

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(new_key.length());
  auto valstring = ss.str();
  size_t valsize = hshm::Unit<size_t>::Bytes(valstring.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  hipc::FullPtr<char> myval =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, valsize);
  char *keyptr = mykey.ptr_;
  char *valptr = myval.ptr_;
  strcpy(keyptr, new_key.c_str());
  memcpy(valptr, valstring.c_str(), valsize);
  client.MetaPut(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_,
	       keysize, myval.shm_, valsize);  

  return 0;
}

int
IOWARPMapImpl::put (const table &name, std::string key, file_stat *fs,
		 std::string group_key)
{
  if (name != table::FILE_DB) {
    DTIO_LOG_ERROR("Attempt to put file stat into unrelated database " << name);
  }
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;

  std::stringstream ss;
  cereal::BinaryOutputArchive ar(ss);
  ar(*fs);

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(new_key.length());
  auto valstring = ss.str();
  size_t valsize = hshm::Unit<size_t>::Bytes(valstring.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  hipc::FullPtr<char> myval =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, valsize);
  char *keyptr = mykey.ptr_;
  char *valptr = myval.ptr_;
  strcpy(keyptr, new_key.c_str());
  memcpy(valptr, valstring.c_str(), valsize);
  client.MetaPut(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_,
	       keysize, myval.shm_, valsize);  

  return 0;
}

int
IOWARPMapImpl::put (const table &name, std::string key, const std::string &value,
                 std::string group_key)
{
  // std::cout << boost::stacktrace::stacktrace();
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(new_key.length());
  size_t valsize = hshm::Unit<size_t>::Bytes(value.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  hipc::FullPtr<char> myval =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, valsize);
  char *keyptr = mykey.ptr_;
  char *valptr = myval.ptr_;
  strcpy(keyptr, new_key.c_str());
  strcpy(valptr, value.c_str());
  printf("Metaput start\n");
  client.MetaPut(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_,
	       keysize, myval.shm_, valsize);
  printf("Metaput done\n");
  return 0;
}

int
IOWARPMapImpl::put (const table &name, std::string key, const char *value, size_t size,
                 std::string group_key)
{
  std::string new_key = std::to_string (name) + KEY_SEPARATOR + key;

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(new_key.length());
  size_t valsize = hshm::Unit<size_t>::Bytes(size);
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  hipc::FullPtr<char> myval =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, valsize);
  char *keyptr = mykey.ptr_;
  char *valptr = myval.ptr_;
  strcpy(keyptr, new_key.c_str());
  memcpy(valptr, value, size);
  client.MetaPut(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_,
	       keysize, myval.shm_, valsize);
  return 0;
}


std::string
IOWARPMapImpl::get (const table &name, std::string key, std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(key.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  char *keyptr = mykey.ptr_;
  strcpy(keyptr, key.c_str());
  printf("Performing metaget\n");
  auto retval = client.MetaGet(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_, keysize);
  printf("Metaget done\n");
  if (std::get<0>(retval)) {
    printf("Get 0 done\n");
    hipc::FullPtr val_full(std::get<1>(retval));
    printf("Get 1 done\n");
    return val_full.ptr_;
  }
  else {
    printf("Get 0 false\n");
    return "";
  }
}

void
IOWARPMapImpl::get (const table &name, std::string key, std::string group_key, char *result, int max_size)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(key.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  char *keyptr = mykey.ptr_;
  strcpy(keyptr, key.c_str());
  auto val = client.MetaGet(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_, keysize);
  if (std::get<0>(val)) {
    hipc::FullPtr val_full(std::get<1>(val));
    memcpy(result, val_full.ptr_, max_size);
  }
}

void
IOWARPMapImpl::get (const table &name, std::string key, std::string group_key, char *result)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(key.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  char *keyptr = mykey.ptr_;
  strcpy(keyptr, key.c_str());
  auto val = client.MetaGet(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_, keysize);
  if (std::get<0>(val)) {
    hipc::FullPtr val_full(std::get<1>(val));
    strcpy(result, val_full.ptr_);
  }
}

bool
IOWARPMapImpl::get (const table &name, std::string key, std::string group_key, chunk_meta *result)
{
  if (name != table::CHUNK_DB) {
    DTIO_LOG_ERROR("Attempt to get chunk meta from unrelated database " << name);
  }

  key = std::to_string (name) + KEY_SEPARATOR + key;
  result = new chunk_meta();

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(key.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  char *keyptr = mykey.ptr_;
  strcpy(keyptr, key.c_str());
  auto val = client.MetaGet(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_, keysize);
  if (std::get<0>(val)) {
    hipc::FullPtr val_full(std::get<1>(val));
    std::stringstream ss(val_full.ptr_);
    cereal::BinaryInputArchive ar(ss);
    ar(*result);
  }
  return std::get<0>(val);
}

bool
IOWARPMapImpl::get (const table &name, std::string key, std::string group_key, file_meta *result)
{
  if (name != table::FILE_CHUNK_DB) {
    DTIO_LOG_ERROR("Attempt to get file chunk meta from unrelated database " << name);
  }

  key = std::to_string (name) + KEY_SEPARATOR + key;
  result = new file_meta();

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(key.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  char *keyptr = mykey.ptr_;
  strcpy(keyptr, key.c_str());
  auto val = client.MetaGet(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_, keysize);
  if (std::get<0>(val)) {
    hipc::FullPtr val_full(std::get<1>(val));
    std::stringstream ss(val_full.ptr_);
    cereal::BinaryInputArchive ar(ss);
    ar(*result);
  }
  return std::get<0>(val);
}

bool
IOWARPMapImpl::get (const table &name, std::string key, std::string group_key, file_stat *result)
{
  printf("filestat get\n");
  if (name != table::FILE_DB) {
    DTIO_LOG_ERROR("Attempt to get file stat from unrelated database " << name);
  }

  key = std::to_string (name) + KEY_SEPARATOR + key;

  result = new file_stat();

  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(key.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  char *keyptr = mykey.ptr_;
  strcpy(keyptr, key.c_str());
  printf("Calling metaget for filestat\n");
  auto val = client.MetaGet(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_, keysize);
  printf("Checking for existence\n");
  if (std::get<0>(val)) {
    printf("Success, input archive\n");
    auto retval = std::get<1>(val);
    hipc::FullPtr val_full(retval);
    const char* data = static_cast<const char*>(val_full.ptr_);
    size_t data_size = strlen(data);
    if (data_size == 0) {
      std::cout << "Empty data received" << std::endl;
    }
    std::stringstream ss;
    ss.write(data, data_size);
    cereal::BinaryInputArchive ar(ss);
    printf("Success?\n");
    try {
      ar(*result);

    } catch (const std::exception& ex) {
      std::cout << "Exception " << ex.what() << std::endl;
    } catch (const std::string& ex) {
      std::cout << "String exception " << ex << std::endl;
    } catch (...) {
      std::cout << "Unknown exception" << std::endl;
    }
    printf("Success 2?\n");
  }
  return std::get<0>(val);
}

std::string
IOWARPMapImpl::remove (const table &name, std::string key, std::string group_key)
{
//   key = std::to_string (name) + KEY_SEPARATOR + key;
//   auto true_key = IOWARPKeyType (key);
//   if (name == table::FILE_DB) {
//     filestat_map->Erase (true_key);
//   }
//   else if (name == table::CHUNK_DB) {
//     chunkmeta_map->Erase (true_key);
//   }
//   else {
// #if(STACK_ALLOCATION)
//     iowarp_client->Erase (true_key);
// #else
//     iowarp_string_client->Erase (true_key);
// #endif
//   }
  // FIXME remove, eventually
  return key;
}

bool
IOWARPMapImpl::exists (const table &name, std::string key, std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t keysize = hshm::Unit<size_t>::Bytes(key.length());
  hipc::FullPtr<char> mykey =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, keysize);
  char *keyptr = mykey.ptr_;
  strcpy(keyptr, key.c_str());
  auto val = client.MetaGet(HSHM_MCTX, chi::DomainQuery::GetDirectHash(chi::SubDomain::kGlobalContainers, 0), mykey.shm_, keysize);

  printf("Exists get 0\n");

  return std::get<0>(val);
}

bool
IOWARPMapImpl::purge ()
{
  // Not necessary, nothing calls it at the moment and memcached just does a
  // flush
}

size_t
IOWARPMapImpl::counter_init (const table &name, std::string key,
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
IOWARPMapImpl::counter_inc (const table &name, std::string key,
                         std::string group_key)
{
  auto keyname = std::to_string (name) + KEY_SEPARATOR + key;

  if (group_key == "-1")
    {
      group_key = get_server (keyname, name);
    }
  printf("Counter inc existence check\n");
  if (this->exists (name, key, group_key))
    {
      printf("Counter inc existence check success\n");
      char *val = (char *)malloc(MAX_IO_UNIT);
      get (name, key, group_key, val);
      printf("Counter inc get done\n");
      size_t numeric_val = atoi (val);
      numeric_val++;
      put (name, key, std::to_string (numeric_val), group_key);
      printf("Counter inc put val done\n");
      delete(val);
      return numeric_val;
    }
  else
    {
      printf("Counter inc existence check fail\n");
      try {
      put (name, key, "0", group_key);
      } catch (const std::exception& ex) {
	std::cout << "Exception " << ex.what() << std::endl;
      } catch (const std::string& ex) {
	std::cout << "String exception " << ex << std::endl;
      } catch (...) {
	std::cout << "Unknown exception" << std::endl;
      }
      printf("put 0 success\n");
      return 0;
    }
}

std::string
IOWARPMapImpl::get_server (std::string key, const table &tab)
{
  // auto true_key = IOWARPKeyType (key);
  // std::hash<IOWARPKeyType> keyHash;
  // keyHash (true_key) % num_servers
  size_t server = 0; //iowarpmapname == "dataspace" ? 0 : 1;
  return std::to_string (server);
}
