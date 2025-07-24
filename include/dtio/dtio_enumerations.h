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

#ifndef DTIO_INCLUDE_DTIO_ENUMERATIONS_H_
#define DTIO_INCLUDE_DTIO_ENUMERATIONS_H_

#include <cstdint>

namespace dtio {

enum class RequestStatus { kCompleted = 0, kPending = 1 };

enum class MessageType { kMetadata = 0, kDataspace = 1 };

enum class Operation { kWrite = 0, kRead = 1, kDelete = 2, kFlush = 3 };

enum class MapType { kMetaFh = 0, kMetaChunk = 1 };

enum class LocationType { kBuffers = 0, kCache = 1, kPfs = 2 };

enum class MpiComms { kWorkerColor = 0, kSchedulerColor = 1, kHclmapColor = 2 };

enum class MpiDataspaceComms { kDataspaceNullColor = 0, kDataspaceColor = 1 };

enum class MpiMetadataComms { kMetadataNullColor = 0, kMetadataColor = 1 };

enum class MpiQueueComms { kQueueWorkerNullColor = 0, kQueueWorkerColor = 1 };

enum class MpiTsComms { kQueueTsNullColor = 0, kQueueTsColor = 1 };

enum class Service {
  kLib = 0,
  kClient = 1,
  kSystemManager = 2,
  kTaskScheduler = 3,
  kWorker = 4,
  kWorkerManager = 5,
  kHclclient = 6
};

enum class TaskType : int64_t {
  kReadTask = 0,
  kWriteTask = 1,
  kFlushTask = 2,
  kDeleteTask = 3,
  kStagingTask = 4,
  kDummy = 5
};

enum class Table {
  kFileDb = 0,
  kFileChunkDb = 1,
  kChunkDb = 2,
  kSystemReg = 3,
  kDataspaceDb = 4,
  kWorkerScore = 5,
  kWorkerCapacity = 6,
  kTaskDb = 7,
  kWriteFinishedDb = 8,
  kCounterDb = 9,
  kStagingDb = 10
};

enum class BuilderImplType { kDefaultB = 0, kAggregatingB = 1 };

enum class MapImplType { kRocksDb = 0, kMemcacheD = 1, kHclmap = 2 };

enum class QueueImplType { kNats = 0, kHclqueue = 1 };

enum class SolverImplType {
  kDp = 0,
  kGreedy = 1,
  kRoundRobin = 2,
  kRandomSelect = 3,
  kDefault = 4
};

// NOTE: kMulti should always be the last interface type, and should not be
// used in tasks. It's used to get the number of interfaces for the purpose of
// the worker-side multi client
enum class IoClientType {
  kStdio = 0,
  kPosix = 1,
  kHdf5 = 2,
  kUring = 3,
  kMulti = 4
};

enum class Distribution { kUniform, kLinear, kExponential };

}  // namespace dtio

#endif  // DTIO_INCLUDE_DTIO_ENUMERATIONS_H_