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

  CHI_BEGIN(Write)
  void Write(WriteTask *task, RunContext &rctx) {
    // So, we should have multiple clients and pass to the appropriate one based off of a DTIO configuration.
    // For now, let's simply assume POSIX. Add more later.

    hipc::FullPtr data_full(task->data_);
    char *data_ = (char *)(data_full.ptr_);

    hipc::FullPtr filename_full(task->filename_);
    char *filepath_ = (char *)(filename_full.ptr_);

    char *filepath = (strncmp(filepath_, "dtio://", 7) == 0) ? (filepath_ + 7) : filepath_;
    int fd;
    // if (temp_fd == -1) {
    fd = open64(filepath, O_RDWR | O_CREAT, 0664); // "w+"
      // temp_fd = fd;
    // }
    // else {
    //   fd = temp_fd;
    // }
    if (fd < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }
    lseek64(fd, task->data_offset_, SEEK_SET);
    auto count = write(fd, data_, task->data_size_);
    if (count != task->data_size_)
      std::cerr << "written less" << count << "\n";

  }

  void MonitorWrite(MonitorModeId mode, WriteTask *task, RunContext &rctx) {
	switch (mode) {
	case MonitorMode::kReplicaAgg: {
	  std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
	}
	}
  }
  CHI_END(Write)

CHI_BEGIN(Read)
  /** The Read method */
  void Read(ReadTask *task, RunContext &rctx) {
  }
  void MonitorRead(MonitorModeId mode, ReadTask *task, RunContext &rctx) {
    switch (mode) {
      case MonitorMode::kReplicaAgg: {
        std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
      }
    }
  }
  CHI_END(Read)
  CHI_AUTOGEN_METHODS  // keep at class bottom
      public:
#include "dt_write/dt_write_lib_exec.h"
};

}  // namespace chi::dt_write

CHI_TASK_CC(chi::dt_write::Server, "dt_write");
