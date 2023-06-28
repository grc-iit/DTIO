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

#include "multi_client.h"
#include <iomanip>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <hcl/common/debug.h>
#include <dtio/dtio_system.h>

int multi_client::dtio_read(read_task task) {
  return clients[task.iface]->dtio_read(task);
}

int multi_client::dtio_write(write_task task) {
  return clients[task.iface]->dtio_write(task);
}

int multi_client::dtio_delete_file(delete_task task) {
  return clients[task.iface]->dtio_delete_file(task);
}

int multi_client::dtio_flush_file(flush_task task) {
  return clients[task.iface]->dtio_flush_file(task);
}
