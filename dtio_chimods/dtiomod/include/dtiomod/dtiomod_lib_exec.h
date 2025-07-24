#ifndef CHI_DTIOMOD_LIB_EXEC_H_
#define CHI_DTIOMOD_LIB_EXEC_H_

/** Execute a task */
void Run(u32 method, Task *task, RunContext &rctx) override {
  switch (method) {
    case Method::kCreate: {
      Create(reinterpret_cast<CreateTask *>(task), rctx);
      break;
    }
    case Method::kDestroy: {
      Destroy(reinterpret_cast<DestroyTask *>(task), rctx);
      break;
    }
    case Method::kWrite: {
      Write(reinterpret_cast<WriteTask *>(task), rctx);
      break;
    }
    case Method::kRead: {
      Read(reinterpret_cast<ReadTask *>(task), rctx);
      break;
    }
    case Method::kPrefetch: {
      Prefetch(reinterpret_cast<PrefetchTask *>(task), rctx);
      break;
    }
    case Method::kMetaPut: {
      MetaPut(reinterpret_cast<MetaPutTask *>(task), rctx);
      break;
    }
    case Method::kMetaGet: {
      MetaGet(reinterpret_cast<MetaGetTask *>(task), rctx);
      break;
    }
    case Method::kSchedule: {
      Schedule(reinterpret_cast<ScheduleTask *>(task), rctx);
      break;
    }
  }
}
/** Execute a task */
void Monitor(MonitorModeId mode, MethodId method, Task *task, RunContext &rctx) override {
  switch (method) {
    case Method::kCreate: {
      MonitorCreate(mode, reinterpret_cast<CreateTask *>(task), rctx);
      break;
    }
    case Method::kDestroy: {
      MonitorDestroy(mode, reinterpret_cast<DestroyTask *>(task), rctx);
      break;
    }
    case Method::kWrite: {
      MonitorWrite(mode, reinterpret_cast<WriteTask *>(task), rctx);
      break;
    }
    case Method::kRead: {
      MonitorRead(mode, reinterpret_cast<ReadTask *>(task), rctx);
      break;
    }
    case Method::kPrefetch: {
      MonitorPrefetch(mode, reinterpret_cast<PrefetchTask *>(task), rctx);
      break;
    }
    case Method::kMetaPut: {
      MonitorMetaPut(mode, reinterpret_cast<MetaPutTask *>(task), rctx);
      break;
    }
    case Method::kMetaGet: {
      MonitorMetaGet(mode, reinterpret_cast<MetaGetTask *>(task), rctx);
      break;
    }
    case Method::kSchedule: {
      MonitorSchedule(mode, reinterpret_cast<ScheduleTask *>(task), rctx);
      break;
    }
  }
}
/** Delete a task */
void Del(const hipc::MemContext &mctx, u32 method, Task *task) override {
  switch (method) {
    case Method::kCreate: {
      CHI_CLIENT->DelTask<CreateTask>(mctx, reinterpret_cast<CreateTask *>(task));
      break;
    }
    case Method::kDestroy: {
      CHI_CLIENT->DelTask<DestroyTask>(mctx, reinterpret_cast<DestroyTask *>(task));
      break;
    }
    case Method::kWrite: {
      CHI_CLIENT->DelTask<WriteTask>(mctx, reinterpret_cast<WriteTask *>(task));
      break;
    }
    case Method::kRead: {
      CHI_CLIENT->DelTask<ReadTask>(mctx, reinterpret_cast<ReadTask *>(task));
      break;
    }
    case Method::kPrefetch: {
      CHI_CLIENT->DelTask<PrefetchTask>(mctx, reinterpret_cast<PrefetchTask *>(task));
      break;
    }
    case Method::kMetaPut: {
      CHI_CLIENT->DelTask<MetaPutTask>(mctx, reinterpret_cast<MetaPutTask *>(task));
      break;
    }
    case Method::kMetaGet: {
      CHI_CLIENT->DelTask<MetaGetTask>(mctx, reinterpret_cast<MetaGetTask *>(task));
      break;
    }
    case Method::kSchedule: {
      CHI_CLIENT->DelTask<ScheduleTask>(mctx, reinterpret_cast<ScheduleTask *>(task));
      break;
    }
  }
}
/** Duplicate a task */
void CopyStart(u32 method, const Task *orig_task, Task *dup_task, bool deep) override {
  switch (method) {
    case Method::kCreate: {
      chi::CALL_COPY_START(
        reinterpret_cast<const CreateTask*>(orig_task), 
        reinterpret_cast<CreateTask*>(dup_task), deep);
      break;
    }
    case Method::kDestroy: {
      chi::CALL_COPY_START(
        reinterpret_cast<const DestroyTask*>(orig_task), 
        reinterpret_cast<DestroyTask*>(dup_task), deep);
      break;
    }
    case Method::kWrite: {
      chi::CALL_COPY_START(
        reinterpret_cast<const WriteTask*>(orig_task), 
        reinterpret_cast<WriteTask*>(dup_task), deep);
      break;
    }
    case Method::kRead: {
      chi::CALL_COPY_START(
        reinterpret_cast<const ReadTask*>(orig_task), 
        reinterpret_cast<ReadTask*>(dup_task), deep);
      break;
    }
    case Method::kPrefetch: {
      chi::CALL_COPY_START(
        reinterpret_cast<const PrefetchTask*>(orig_task), 
        reinterpret_cast<PrefetchTask*>(dup_task), deep);
      break;
    }
    case Method::kMetaPut: {
      chi::CALL_COPY_START(
        reinterpret_cast<const MetaPutTask*>(orig_task), 
        reinterpret_cast<MetaPutTask*>(dup_task), deep);
      break;
    }
    case Method::kMetaGet: {
      chi::CALL_COPY_START(
        reinterpret_cast<const MetaGetTask*>(orig_task), 
        reinterpret_cast<MetaGetTask*>(dup_task), deep);
      break;
    }
    case Method::kSchedule: {
      chi::CALL_COPY_START(
        reinterpret_cast<const ScheduleTask*>(orig_task), 
        reinterpret_cast<ScheduleTask*>(dup_task), deep);
      break;
    }
  }
}
/** Duplicate a task */
void NewCopyStart(u32 method, const Task *orig_task, FullPtr<Task> &dup_task, bool deep) override {
  switch (method) {
    case Method::kCreate: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const CreateTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kDestroy: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const DestroyTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kWrite: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const WriteTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kRead: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const ReadTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kPrefetch: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const PrefetchTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kMetaPut: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const MetaPutTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kMetaGet: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const MetaGetTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kSchedule: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const ScheduleTask*>(orig_task), dup_task, deep);
      break;
    }
  }
}
/** Serialize a task when initially pushing into remote */
void SaveStart(
    u32 method, BinaryOutputArchive<true> &ar,
    Task *task) override {
  switch (method) {
    case Method::kCreate: {
      ar << *reinterpret_cast<CreateTask*>(task);
      break;
    }
    case Method::kDestroy: {
      ar << *reinterpret_cast<DestroyTask*>(task);
      break;
    }
    case Method::kWrite: {
      ar << *reinterpret_cast<WriteTask*>(task);
      break;
    }
    case Method::kRead: {
      ar << *reinterpret_cast<ReadTask*>(task);
      break;
    }
    case Method::kPrefetch: {
      ar << *reinterpret_cast<PrefetchTask*>(task);
      break;
    }
    case Method::kMetaPut: {
      ar << *reinterpret_cast<MetaPutTask*>(task);
      break;
    }
    case Method::kMetaGet: {
      ar << *reinterpret_cast<MetaGetTask*>(task);
      break;
    }
    case Method::kSchedule: {
      ar << *reinterpret_cast<ScheduleTask*>(task);
      break;
    }
  }
}
/** Deserialize a task when popping from remote queue */
TaskPointer LoadStart(    u32 method, BinaryInputArchive<true> &ar) override {
  TaskPointer task_ptr;
  switch (method) {
    case Method::kCreate: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<CreateTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<CreateTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kDestroy: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<DestroyTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<DestroyTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kWrite: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<WriteTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<WriteTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kRead: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<ReadTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<ReadTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kPrefetch: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<PrefetchTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<PrefetchTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kMetaPut: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<MetaPutTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<MetaPutTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kMetaGet: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<MetaGetTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<MetaGetTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kSchedule: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<ScheduleTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<ScheduleTask*>(task_ptr.ptr_);
      break;
    }
  }
  return task_ptr;
}
/** Serialize a task when returning from remote queue */
void SaveEnd(u32 method, BinaryOutputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kCreate: {
      ar << *reinterpret_cast<CreateTask*>(task);
      break;
    }
    case Method::kDestroy: {
      ar << *reinterpret_cast<DestroyTask*>(task);
      break;
    }
    case Method::kWrite: {
      ar << *reinterpret_cast<WriteTask*>(task);
      break;
    }
    case Method::kRead: {
      ar << *reinterpret_cast<ReadTask*>(task);
      break;
    }
    case Method::kPrefetch: {
      ar << *reinterpret_cast<PrefetchTask*>(task);
      break;
    }
    case Method::kMetaPut: {
      ar << *reinterpret_cast<MetaPutTask*>(task);
      break;
    }
    case Method::kMetaGet: {
      ar << *reinterpret_cast<MetaGetTask*>(task);
      break;
    }
    case Method::kSchedule: {
      ar << *reinterpret_cast<ScheduleTask*>(task);
      break;
    }
  }
}
/** Deserialize a task when popping from remote queue */
void LoadEnd(u32 method, BinaryInputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kCreate: {
      ar >> *reinterpret_cast<CreateTask*>(task);
      break;
    }
    case Method::kDestroy: {
      ar >> *reinterpret_cast<DestroyTask*>(task);
      break;
    }
    case Method::kWrite: {
      ar >> *reinterpret_cast<WriteTask*>(task);
      break;
    }
    case Method::kRead: {
      ar >> *reinterpret_cast<ReadTask*>(task);
      break;
    }
    case Method::kPrefetch: {
      ar >> *reinterpret_cast<PrefetchTask*>(task);
      break;
    }
    case Method::kMetaPut: {
      ar >> *reinterpret_cast<MetaPutTask*>(task);
      break;
    }
    case Method::kMetaGet: {
      ar >> *reinterpret_cast<MetaGetTask*>(task);
      break;
    }
    case Method::kSchedule: {
      ar >> *reinterpret_cast<ScheduleTask*>(task);
      break;
    }
  }
}

#endif  // CHI_DTIOMOD_LIB_EXEC_H_
