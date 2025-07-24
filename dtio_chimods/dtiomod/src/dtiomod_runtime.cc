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

#include "chimaera/api/chimaera_runtime.h"
#include "chimaera/monitor/monitor.h"
#include "chimaera_admin/chimaera_admin_client.h"
#include "dtio/dtio_enumerations.h"
#include "dtiomod/dtiomod_client.h"

namespace chi::dtiomod {

class Server : public Module {
 public:
  CLS_CONST LaneGroupId kDefaultGroup = 0;

 public:
  std::unordered_map<std::string, std::string> metamap;
  size_t schedule_num;

  Server() = default;

  CHI_BEGIN(Create)
  /** Construct dtiomod */
  void Create(CreateTask *task, RunContext &rctx) {
    // Create a set of lanes for holding tasks
    schedule_num = 0;
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
  /** Destroy dtiomod */
  void Destroy(DestroyTask *task, RunContext &rctx) {}
  void MonitorDestroy(MonitorModeId mode, DestroyTask *task, RunContext &rctx) {
  }
  CHI_END(Destroy)

  CHI_BEGIN(Write)
  void Write(WriteTask *task, RunContext &rctx) {
    printf("DTIO chimod write\n");
    // So, we should have multiple clients and pass to the appropriate one based
    // off of a DTIOMOD configuration. For now, let's simply assume POSIX. Add
    // more later.

    hipc::FullPtr data_full(task->data_);
    char *data_ = (char *)(data_full.ptr_);

    std::string filepath_str = task->filename_.str();
    std::string filepath = (filepath_str.compare(0, 7, "dtio://") == 0)
                               ? filepath_str.substr(7)
                               : filepath_str;

    switch (task->iface_) {
      case dtio::IoClientType::kPosix: {
        int fd;
        // if (temp_fd == -1) {
        fd = open64(filepath.c_str(), O_RDWR | O_CREAT, 0664);  // "w+"
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

      } break;
      case dtio::IoClientType::kStdio: {
        FILE *fp;
        // if (temp_fd == -1) {
        fp = fopen(filepath.c_str(), "rw+");  // "w+"
                                              // temp_fd = fd;
        // }
        // else {
        //   fd = temp_fd;
        // }
        if (fp == nullptr) {
          std::cerr << "File " << filepath << " didn't open" << std::endl;
        }
        fseek(fp, task->data_offset_, SEEK_SET);
        auto count = fwrite(data_, sizeof(char), task->data_size_, fp);
        if (count != task->data_size_)
          std::cerr << "written less" << count << "\n";

      } break;
    }
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
    // So, we should have multiple clients and pass to the appropriate one based
    // off of a DTIOMOD configuration. For now, let's simply assume POSIX. Add
    // more later.

    hipc::FullPtr data_full(task->data_);
    char *data_ = (char *)(data_full.ptr_);

    std::string filepath_str = task->filename_.str();
    std::string filepath = (filepath_str.compare(0, 7, "dtio://") == 0)
                               ? filepath_str.substr(7)
                               : filepath_str;

    switch (task->iface_) {
      case dtio::IoClientType::kPosix: {
        int fd;
        // if (temp_fd == -1) {
        fd = open64(filepath.c_str(), O_RDWR | O_CREAT, 0664);  // "w+"
                                                                // temp_fd = fd;
        // }
        // else {
        //   fd = temp_fd;
        // }
        if (fd < 0) {
          std::cerr << "File " << filepath << " didn't open" << std::endl;
        }
        lseek64(fd, task->data_offset_, SEEK_SET);
        auto count = read(fd, data_, task->data_size_);
        if (count != task->data_size_)
          std::cerr << "read less" << count << "\n";

      } break;
      case dtio::IoClientType::kStdio: {
        FILE *fp;
        // if (temp_fd == -1) {
        fp = fopen(filepath.c_str(), "rw+");  // "w+"
                                              // temp_fd = fd;
        // }
        // else {
        //   fd = temp_fd;
        // }
        if (fp == nullptr) {
          std::cerr << "File " << filepath << " didn't open" << std::endl;
        }
        fseek(fp, task->data_offset_, SEEK_SET);
        auto count = fread(data_, sizeof(char), task->data_size_, fp);
        if (count != task->data_size_)
          std::cerr << "read less" << count << "\n";

      } break;
    }
  }

  void MonitorRead(MonitorModeId mode, ReadTask *task, RunContext &rctx) {
    switch (mode) {
      case MonitorMode::kReplicaAgg: {
        std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
      }
    }
  }
  CHI_END(Read)

  CHI_BEGIN(Prefetch)
  /** The Prefetch method */
  void Prefetch(PrefetchTask *task, RunContext &rctx) {}
  void MonitorPrefetch(MonitorModeId mode, PrefetchTask *task,
                       RunContext &rctx) {
    switch (mode) {
      case MonitorMode::kReplicaAgg: {
        std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
      }
    }
  }
  CHI_END(Prefetch)

  template <typename TaskT>
  void IoRoute(TaskT *task) {
    // Concretize the domain to map the task
    task->dom_query_ =
        chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0);
    task->SetDirect();
    task->UnsetRouted();
  }

  CHI_BEGIN(MetaPut)
  /** The MetaPut method */
  void MetaPut(MetaPutTask *task, RunContext &rctx) {
    std::string key = task->key_.str();
    std::string val = task->val_.str();
    metamap[key] = val;
    printf("Metaput runtime put done\n");
  }
  void MonitorMetaPut(MonitorModeId mode, MetaPutTask *task, RunContext &rctx) {
    switch (mode) {
      case MonitorMode::kReplicaAgg: {
        std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
      }
      case MonitorMode::kSchedule: {
        IoRoute<MetaPutTask>(task);
        return;
      }
    }
  }
  CHI_END(MetaPut)

  CHI_BEGIN(MetaGet)
  /** The MetaGet method */
  void MetaGet(MetaGetTask *task, RunContext &rctx) {
    std::string key = task->key_.str();
    task->presence_ = (metamap.find(key) != metamap.end());
    if (!task->presence_) {
      std::cout << "DTIOMOD key not found" << std::endl;
      return;
    }
    std::cout << "DTIOMOD key found" << std::endl;
    std::string val = metamap[key];
    task->val_ = val;
    std::cout << "DTIOMOD strlen val " << val.length() << std::endl;
  }
  void MonitorMetaGet(MonitorModeId mode, MetaGetTask *task, RunContext &rctx) {
    switch (mode) {
      case MonitorMode::kReplicaAgg: {
        std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
      }
      case MonitorMode::kSchedule: {
        IoRoute<MetaGetTask>(task);
        return;
      }
    }
  }
  CHI_END(MetaGet)

  CHI_BEGIN(Schedule)
  /** The Schedule method */
  void Schedule(ScheduleTask *task, RunContext &rctx) {
    task->schedule_num_ = schedule_num++;
  }
  void MonitorSchedule(MonitorModeId mode, ScheduleTask *task,
                       RunContext &rctx) {
    switch (mode) {
      case MonitorMode::kReplicaAgg: {
        std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
      }
    }
  }
  CHI_END(Schedule)
  CHI_AUTOGEN_METHODS  // keep at class bottom
      public:
#include "dtiomod/dtiomod_lib_exec.h"
};

}  // namespace chi::dtiomod

CHI_TASK_CC(chi::dtiomod::Server, "dtiomod");
