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

#ifndef DTIO_MAIN_ENUMERATION_H
#define DTIO_MAIN_ENUMERATION_H

#include <cstdint>

enum request_status { COMPLETED = 0, PENDING = 1 };

enum message_type { METADATA = 0, DATASPACE = 1 };
enum operation { WRITE = 0, READ = 1, DELETE = 2, FLUSH = 3 };
enum map_type { META_FH = 0, META_CHUNK = 1 };
enum location_type { BUFFERS = 0, CACHE = 1, PFS = 2 };
enum mpi_comms { WORKER_COLOR = 0, SCHEDULER_COLOR = 1, HCLMAP_COLOR = 2 };
enum mpi_dataspace_comms { DATASPACE_NULL_COLOR = 0, DATASPACE_COLOR = 1 };
enum mpi_metadata_comms { METADATA_NULL_COLOR = 0, METADATA_COLOR = 1 };

enum mpi_queue_comms { QUEUE_WORKER_NULL_COLOR = 0, QUEUE_WORKER_COLOR = 1 };

enum mpi_ts_comms { QUEUE_TS_NULL_COLOR = 0, QUEUE_TS_COLOR = 1 };

enum service {
  LIB = 0,
  CLIENT = 1,
  SYSTEM_MANAGER = 2,
  TASK_SCHEDULER = 3,
  WORKER = 4,
  WORKER_MANAGER = 5,
  HCLCLIENT = 6
};
enum class task_type : int64_t {
  READ_TASK = 0,
  WRITE_TASK = 1,
  FLUSH_TASK = 2,
  DELETE_TASK = 3,
  STAGING_TASK = 4,
  DUMMY = 5
};

enum table {
  FILE_DB = 0,
  FILE_CHUNK_DB = 1,
  CHUNK_DB = 2,
  SYSTEM_REG = 3,
  DATASPACE_DB = 4,
  WORKER_SCORE = 5,
  WORKER_CAPACITY = 6,
  TASK_DB = 7,
  WRITE_FINISHED_DB = 8,
  COUNTER_DB = 9,
  STAGING_DB = 10
};
enum builder_impl_type { DEFAULT_B = 0, AGGREGATING_B = 1 };
enum map_impl_type { ROCKS_DB = 0, MEMCACHE_D = 1, IOWARP = 2 };
enum queue_impl_type { NATS = 0, HCLQUEUE = 1 };
enum solver_impl_type {
  DP = 0,
  GREEDY = 1,
  ROUND_ROBIN = 2,
  RANDOM_SELECT = 3,
  DEFAULT = 4
};

// NOTE: MULTI should always be the last interface type, and should not be used
// in tasks It's used to get the number of interfaces for the purpose of the
// worker-side multi client
enum io_client_type { STDIO = 0, POSIX = 1, HDF5 = 2, URING = 3, MULTI = 4 };

enum Distribution { UNIFORM, LINEAR, EXPONENTIAL };

#endif  // DTIO_MAIN_ENUMERATION_H
