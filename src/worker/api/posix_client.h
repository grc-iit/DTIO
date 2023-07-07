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
//
// Created by hdevarajan on 5/10/18.
//

#ifndef DTIO_MAIN_POSIXCLIENT_H
#define DTIO_MAIN_POSIXCLIENT_H

#include "io_client.h"
#include "dtio/common/config_manager.h"
#include <chrono>
#include <dtio/common/data_structures.h>

using namespace std::chrono;

class posix_client : public io_client {
  std::string dir;

public:
  posix_client(int worker_index) : io_client(worker_index) {
    dir = ConfigManager::get_instance()->WORKER_PATH + "/" +
          std::to_string(worker_index) + "/";
  }
  int dtio_write(write_task task) override;
  int dtio_read(read_task task) override;
  int dtio_delete_file(delete_task task) override;
  int dtio_flush_file(flush_task task) override;
};

#endif // DTIO_MAIN_POSIXCLIENT_H
