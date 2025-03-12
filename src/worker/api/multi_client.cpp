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

#include "multi_client.h"
#include <iomanip>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <hcl/common/debug.h>
#include <dtio/dtio_system.h>

int multi_client::dtio_stage(task *tsk[], char *staging_space) {
  return clients[tsk[0]->iface]->dtio_stage(tsk, staging_space);
}

int multi_client::dtio_read(task *tsk[], char *staging_space) {
  return clients[tsk[0]->iface]->dtio_read(tsk, staging_space);
}

int multi_client::dtio_write(task *tsk[]) {
  return clients[tsk[0]->iface]->dtio_write(tsk);
}

int multi_client::dtio_delete_file(task *tsk[]) {
  return clients[tsk[0]->iface]->dtio_delete_file(tsk);
}

int multi_client::dtio_flush_file(task *tsk[]) {
  return clients[tsk[0]->iface]->dtio_flush_file(tsk);
}
