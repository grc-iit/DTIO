/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dt_write/dt_write_client.h"
#include "chimaera/api/chimaera_runtime.h"
#include "chimaera/monitor/monitor.h"
#include "chimaera_admin/chimaera_admin_client.h"

namespace chi::dt_write {

class Server : public Module {
 public:
  CLS_CONST LaneGroupId kDefaultGroup = 0;

 public:
  Server() = default;

  CHI_BEGIN(Create)
  /** Construct dt_write */
  void Create(CreateTask *task, RunContext &rctx) {
    // Create a set of lanes for holding tasks
    CreateLaneGroup(kDefaultGroup, 1, QUEUE_LOW_LATENCY);
  }
  void MonitorCreate(MonitorModeId mode, CreateTask *task, RunContext &rctx) {}
  CHI_END(Create)

  /** Route a task to a lane */
  Lane *MapTaskToLane(const Task *task) override {
    // Route tasks to lanes based on their properties
    // E.g., a strongly consistent filesystem could map tasks to a lane
    // by the hash of an absolute filename path.
    return GetLaneByHash(kDefaultGroup, task->prio_, 0);
  }

  CHI_BEGIN(Destroy)
  /** Destroy dt_write */
  void Destroy(DestroyTask *task, RunContext &rctx) {}
  void MonitorDestroy(MonitorModeId mode, DestroyTask *task, RunContext &rctx) {
  }
  CHI_END(Destroy)

  CHI_AUTOGEN_METHODS  // keep at class bottom
      public:
#include "dt_write/dt_write_lib_exec.h"
};

}  // namespace chi::dt_write

CHI_TASK_CC(chi::dt_write::Server, "dt_write");
