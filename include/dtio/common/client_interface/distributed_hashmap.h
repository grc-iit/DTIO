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
#ifndef DTIO_MAIN_DISTRIBUTEDHASHMAP_H
#define DTIO_MAIN_DISTRIBUTEDHASHMAP_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <dtio/common/enumerations.h>
#include <dtio/common/data_structures.h>
#include <dtio/common/constants.h>
#include <cereal/types/memory.hpp>
#include <dtio/common/exceptions.h>
/******************************************************************************
 *Class
 ******************************************************************************/
class distributed_hashmap {
protected:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  service service_i;

public:
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  explicit distributed_hashmap(service service) : service_i(service) {}
  /******************************************************************************
   *Interface
   ******************************************************************************/
  virtual int put(const table &name, std::string key, const std::string &value,
                  std::string group_key) {
    throw NotImplementedException("put");
  }
  virtual int put (const table &name, std::string key, const char *value, size_t size,
		   std::string group_key) {
    throw NotImplementedException("put 2");
  }
  virtual int put (const table &name, std::string key, chunk_meta *value,
		   std::string group_key) {
    throw NotImplementedException("put chunkmeta");
  }
  virtual int put (const table &name, std::string key, file_stat *value,
		   std::string group_key) {
    throw NotImplementedException("put filestat");
  }

  virtual std::string get(const table &name, std::string key,
                          std::string group_key) {
    throw NotImplementedException("get");
  }
  virtual void get(const table &name, std::string key,
		   std::string group_key, char *result) {
    throw NotImplementedException("get 2");
  }

  virtual bool get (const table &name, std::string key,
		    std::string group_key, chunk_meta *result) {
    throw NotImplementedException("get chunkmeta");
  }

  virtual bool get (const table &name, std::string key,
		    std::string group_key, file_stat *result) {
    throw NotImplementedException("get filestat");
  }

  virtual std::string remove(const table &name, std::string key,
                             std::string group_key) {
    throw NotImplementedException("remove");
  }
  virtual bool exists(const table &name, std::string key,
                      std::string group_key) {
    throw NotImplementedException("remove");
  }
  virtual bool purge() { throw NotImplementedException("purge"); }
  virtual size_t counter_init(const table &name, std::string key,
                              std::string group_key) {
    throw NotImplementedException("counter_init");
  }
  virtual size_t counter_inc(const table &name, std::string key,
                             std::string group_key) {
    throw NotImplementedException("counter_inc");
  }
  virtual size_t get_servers() { throw NotImplementedException("get_servers"); }
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~distributed_hashmap() {}
};

#endif // DTIO_MAIN_DISTRIBUTEDHASHMAP_H
