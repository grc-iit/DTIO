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
#ifndef DTIO_MAIN_CONSTANTS_H
#define DTIO_MAIN_CONSTANTS_H

#include <climits>
#include <cstddef>
#include <dtio/common/enumerations.h>
#include <string>

// DTIO parameters
const int BATCH_SIZE = 1;
const std::string DTIO_CLIENT_PORT = "9999";
const size_t KEY_SIZE = 256;
const size_t FILE_SIZE = 256;
const long long MAX_DATA_SIZE = 2ul * 1024ul * 1024ul * 1024ul;
const size_t CHUNK_SIZE = 2 * 1024 * 1024;
const size_t CHUNK_LIMIT = 1050;
const long long MAX_MESSAGE_SIZE = LLONG_MAX;
const std::string ALL_KEYS = "ALL";
const std::string kDBPath_client = "/tmp/rocksdb";
const std::string kDBPath_server = "/tmp/rocksdb";
const std::size_t MIN_IO_UNIT = 8 * 1024;
const std::size_t MAX_IO_UNIT
    = 32 * 1024
      * 1024; // Recommendation: Do not set above 2 MB with stack allocation,
              // switch to heap allocation if your program is memory-intensive.
              // Never set above 64 MB, that's as far as strings go
// TODO investigate why 64 MB is the limit. Should be allowed to go up to 128
// MB but we're not. Why?
#define STACK_ALLOCATION false
const std::string CLIENT_TASK_SUBJECT = "TASK";
#define DTIO_FILENAME_MAX 75
// FILENAME_MAX

// Configs
const map_impl_type map_impl_type_t = map_impl_type::IOWARP;
const builder_impl_type builder_impl_type_t = builder_impl_type::DEFAULT_B;
const solver_impl_type solver_impl_type_t = solver_impl_type::ROUND_ROBIN;
// const queue_impl_type queue_impl_type_t = queue_impl_type::HCLQUEUE;
const io_client_type io_client_type_t = io_client_type::POSIX; // HDF5
const std::string DATASPACE_ID = "DATASPACE_ID";
const std::string ROUND_ROBIN_INDEX = "ROUND_ROBIN_INDEX";
const std::string KEY_SEPARATOR = "#";
const size_t PROCS_PER_MEMCACHED = 8;

// Workers
const int WORKER_SPEED = 2;
const int WORKER_ENERGY = 2;
const int64_t WORKER_CAPACITY_MAX = 137438953472;
const size_t KB = 1024;
const std::size_t WORKER_ATTRIBUTES_COUNT = 5;
const float POLICY_WEIGHT[WORKER_ATTRIBUTES_COUNT] = { .3, .2, .3, .1, .1 };
const double WORKER_INTERVAL = 2.0;
const std::size_t MAX_WORKER_TASK_COUNT = 50;

// Worker Manager
const int MAX_SCORE = 100;
const int NUM_BUCKETS = 2; // % of buckets to use for sorting
const Distribution distribution
    = UNIFORM; // Way to distribute workers to buckets

// Scheduler
const std::size_t MAX_NUM_TASKS_IN_QUEUE = 1;
const std::size_t MAX_SCHEDULE_TIMER = 1;
const std::size_t MAX_READ_TIMER = 3;
const std::size_t MAX_TASK_TIMER_MS = MAX_SCHEDULE_TIMER * 1000;
const std::size_t MAX_TASK_TIMER_MS_MAX = MAX_SCHEDULE_TIMER * 1000000;
const std::size_t WORKER_MANAGER_INTERVAL = 5;
const std::size_t SYSTEM_MANAGER_INTERVAL = 5;

#endif // DTIO_MAIN_CONSTANTS_H
