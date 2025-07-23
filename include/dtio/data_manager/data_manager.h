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
#ifndef DTIO_MAIN_DATA_MANAGER_H
#define DTIO_MAIN_DATA_MANAGER_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <cereal/types/memory.hpp>
#include <dtio/client_interface/distributed_hashmap.h>
#include <dtio/dtio_system.h>
#include <dtio/enumerations.h>
/******************************************************************************
 *Class
 ******************************************************************************/
class data_manager
{
private:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  static std::shared_ptr<data_manager> instance;
  service service_i;
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  explicit data_manager (service service) : service_i (service) {}

public:
  /******************************************************************************
   *Interface
   ******************************************************************************/
  inline static std::shared_ptr<data_manager>
  getInstance (service service)
  {
    return instance == nullptr
               ? instance
                 = std::shared_ptr<data_manager> (new data_manager (service))
               : instance;
  }
  std::string get (const table &name, std::string key, std::string server);
  void get (const table &name, std::string key, std::string server,
            char *result);
  int put (const table &name, std::string key, const char *data, int size,
           std::string server);
  int put (const table &name, std::string key, std::string data,
           std::string server);
  bool exists (const table &name, std::string key, std::string server);
  std::string remove (const table &name, std::string key, std::string server);
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~data_manager () {}
};
#endif // DTIO_MAIN_DATA_MANAGER_H
