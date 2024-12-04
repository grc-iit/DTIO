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

#ifndef DTIO_MAIN_MULTICLIENT_H
#define DTIO_MAIN_MULTICLIENT_H

#include "io_client.h"
#include "posix_client.h"
#include "stdio_client.h"
#include "uring_client.h"
#include "hdf5_client.h"
#include "dtio/common/config_manager.h"
#include <chrono>
#include <dtio/common/data_structures.h>

using namespace std::chrono;

class multi_client : public io_client {
  std::string dir;
  std::shared_ptr<io_client> *clients;

public:
  // The multi client is basically a passthrough to other clients.
  // It uses the task iface to determine which client to use
  multi_client(int worker_index) : io_client(worker_index) {
    dir = ConfigManager::get_instance()->WORKER_PATH + "/" +
          std::to_string(worker_index) + "/";
    clients = (std::shared_ptr<io_client> *)calloc(io_client_type::MULTI, sizeof(std::shared_ptr<io_client>));
    clients[io_client_type::POSIX] = std::make_shared<posix_client>(worker_index);
    clients[io_client_type::STDIO] = std::make_shared<stdio_client>(worker_index);
    clients[io_client_type::URING] = std::make_shared<uring_client>(worker_index);
    clients[io_client_type::HDF5] = std::make_shared<hdf5_client>(worker_index);
  }
  int dtio_write(task *tsk[]) override;
  int dtio_read(task *tsk[]) override;
  int dtio_delete_file(task *tsk[]) override;
  int dtio_flush_file(task *tsk[]) override;

  virtual ~multi_client() { free(clients); }
};

#endif // DTIO_MAIN_MULTICLIENT_H
