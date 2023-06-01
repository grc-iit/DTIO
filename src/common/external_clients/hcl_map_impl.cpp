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
#include <dtio/common/external_clients/hcl_map_impl.h>
#include <mpi.h>

int HCLMapImpl::put(const table &name, std::string key,
                       const std::string &value, std::string group_key) {
  return hcl_client->Put(key, value);
}

std::string HCLMapImpl::get(const table &name, std::string key,
                               std::string group_key) {
  return hcl_client->Get(key);
}

std::string HCLMapImpl::remove(const table &name, std::string key,
                                  std::string group_key) {
  hcl_client->Erase(key);
}

bool HCLMapImpl::exists(const table &name, std::string key,
                           std::string group_key) {
}

bool HCLMapImpl::purge() { }

size_t HCLMapImpl::counter_init(const table &name, std::string key,
                                   std::string group_key) {
}

size_t HCLMapImpl::counter_inc(const table &name, std::string key,
                                  std::string group_key) {
}

std::string HCLMapImpl::get_server(std::string key) {
}
