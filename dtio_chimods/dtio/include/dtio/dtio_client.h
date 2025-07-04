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

#ifndef CHI_dtio_H_
#define CHI_dtio_H_

#include "dtio_tasks.h"

namespace chi::dtio {

/** Create dtio requests */
class Client : public ModuleClient {
 public:
  /** Default constructor */
  HSHM_INLINE_CROSS_FUN
  Client() = default;

  /** Destructor */
  HSHM_INLINE_CROSS_FUN
  ~Client() = default;

  CHI_BEGIN(Create)
  /** Create a pool */
  HSHM_INLINE_CROSS_FUN
  void Create(const hipc::MemContext &mctx, const DomainQuery &dom_query,
              const DomainQuery &affinity, const chi::string &pool_name,
              const CreateContext &ctx = CreateContext(), int dtio_id = 0) {
    FullPtr<CreateTask> task =
      AsyncCreate(mctx, dom_query, affinity, pool_name, ctx, dtio_id);
    task->Wait();
    Init(task->ctx_.id_);
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(Create);
  CHI_END(Create)

  CHI_BEGIN(Destroy)
  /** Destroy pool + queue */
  HSHM_INLINE_CROSS_FUN
  void Destroy(const hipc::MemContext &mctx, const DomainQuery &dom_query) {
    FullPtr<DestroyTask> task = AsyncDestroy(mctx, dom_query, id_);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(Destroy)
  CHI_END(Destroy)

  CHI_BEGIN(Write)
  /** Write task */
  void Write(const hipc::MemContext &mctx, const DomainQuery &dom_query,
	     const hipc::Pointer &data, size_t data_size, size_t data_offset, const hipc::Pointer &filename, size_t filenamelen, io_client_type iface) {
    FullPtr<WriteTask> task = AsyncWrite(mctx, dom_query, data, data_size, data_offset, filename, filenamelen, iface);
	task->Wait();
	CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(Write);
  CHI_END(Write)

CHI_BEGIN(Read)
  /** Read task */
  void Read(const hipc::MemContext &mctx,
	    const DomainQuery &dom_query,
	    const hipc::Pointer &data, size_t data_size, size_t data_offset, const hipc::Pointer &filename, size_t filenamelen, io_client_type iface) {
    FullPtr<ReadTask> task =
      AsyncRead(mctx, dom_query, data, data_size, data_offset, filename, filenamelen, iface);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(Read);
  CHI_END(Read)

CHI_BEGIN(Prefetch)
  /** Prefetch task */
  void Prefetch(const hipc::MemContext &mctx,
                      const DomainQuery &dom_query) {
    FullPtr<PrefetchTask> task =
      AsyncPrefetch(mctx, dom_query);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(Prefetch);
  CHI_END(Prefetch)

CHI_BEGIN(MetaPut)
  /** MetaPut task */
  void MetaPut(const hipc::MemContext &mctx,
                      const DomainQuery &dom_query) {
    FullPtr<MetaPutTask> task =
      AsyncMetaPut(mctx, dom_query);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(MetaPut);
  CHI_END(MetaPut)

CHI_BEGIN(MetaGet)
  /** MetaGet task */
  void MetaGet(const hipc::MemContext &mctx,
                      const DomainQuery &dom_query) {
    FullPtr<MetaGetTask> task =
      AsyncMetaGet(mctx, dom_query);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(MetaGet);
  CHI_END(MetaGet)

  CHI_AUTOGEN_METHODS  // keep at class bottom
};

}  // namespace chi::dtio

#endif  // CHI_dtio_H_
