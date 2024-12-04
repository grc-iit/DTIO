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

#ifndef DTIO_MAIN_HDF5CLIENT_H
#define DTIO_MAIN_HDF5CLIENT_H

#include "io_client.h"
#include "dtio/common/config_manager.h"
#include <chrono>
#include <dtio/common/data_structures.h>
#include <hdf5.h>
#include <cmath>

using namespace std::chrono;

class hdf5_client : public io_client {
  std::string dir;
  hid_t temp_fd;

public:
  hdf5_client(int worker_index) : io_client(worker_index) {
    dir = ConfigManager::get_instance()->WORKER_PATH + "/" +
          std::to_string(worker_index) + "/";
    temp_fd = -1;
  }
  int dtio_write(task *tsk[]) override;
  int dtio_read(task *tsk[]) override;
  int dtio_delete_file(task *tsk[]) override;
  int dtio_flush_file(task *tsk[]) override;
  ~hdf5_client() {
    if (temp_fd != -1) {
      H5Fclose(temp_fd);
    }
  }
};

#endif // DTIO_MAIN_HDF5CLIENT_H
