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
#include "dtio/common/constants.h"
#include <dtio/common/external_clients/hcl_map_impl.h>
#include <mpi.h>

int
HCLMapImpl::run()
{
  while (true); // Busy loop, the maps just support the data structures
}

int
HCLMapImpl::put (const table &name, std::string key, const std::string &value,
                 std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (key);
  DTIO_LOG_DEBUG("Put in " << hclmapname << " at " << key << " " << value);
  auto true_val = DTIOCharStruct(value);
  if (hcl_client->Put (true_key, true_val))
    {
      return 0;
    }
  else
    {
      std::cerr << "put failed for key:" << key << "\n";
      return 1;
    }
}

std::string
HCLMapImpl::get (const table &name, std::string key, std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  DTIO_LOG_DEBUG("Get in " << hclmapname << " at " << key);
  auto true_key = HCLKeyType (key);
  auto retval = hcl_client->Get (true_key);
  if (retval.first)
    {
      return retval.second.string();
    }
  else
    {
      return "";
    }
}

std::string
HCLMapImpl::remove (const table &name, std::string key, std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (key);
  hcl_client->Erase (true_key);
  return key;
}

bool
HCLMapImpl::exists (const table &name, std::string key, std::string group_key)
{
  key = std::to_string (name) + KEY_SEPARATOR + key;
  auto true_key = HCLKeyType (key);
  auto retval = hcl_client->Get (true_key);
  return retval.first;
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
      group_key = get_server (key);
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
      group_key = get_server (keyname);
    }
  if (exists (name, key, group_key))
    {
      auto val = get (name, key, group_key);
      size_t numeric_val = atoi (val.c_str ());
      numeric_val++;
      put (name, key, std::to_string (numeric_val), group_key);
      return numeric_val;
    }
  else
    {
      put (name, key, "0", group_key);
      return 0;
    }
}

std::string
HCLMapImpl::get_server (std::string key)
{
  auto true_key = HCLKeyType (key);
  std::hash<HCLKeyType> keyHash;
  size_t server = keyHash (true_key) % num_servers;
  return std::to_string (server);
}
