//
// Created by lukemartinlogan on 8/11/23.
//

#ifndef CHI_TASKS_TASK_TEMPL_INCLUDE_dt_write_dt_write_TASKS_H_
#define CHI_TASKS_TASK_TEMPL_INCLUDE_dt_write_dt_write_TASKS_H_

#include "chimaera/chimaera_namespace.h"

namespace chi::dt_write {

#include "dt_write_methods.h"
CHI_NAMESPACE_INIT

CHI_BEGIN(Create)
/** A task to create dt_write */
struct CreateTaskParams {
  CLS_CONST char *lib_name_ = "example_dt_write";
  int dtio_id_;

  HSHM_INLINE_CROSS_FUN
  CreateTaskParams() = default;

  HSHM_INLINE_CROSS_FUN
  CreateTaskParams(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
		  int dtio_id = 0) {
	  dtio_id_ = dtio_id;
  }

  template <typename Ar>
  HSHM_INLINE_CROSS_FUN void serialize(Ar &ar) {
    ar(dtio_id_);
  }
};
typedef chi::Admin::CreatePoolBaseTask<CreateTaskParams> CreateTask;
CHI_END(Create)

CHI_BEGIN(Destroy)
/** A task to destroy dt_write */
typedef chi::Admin::DestroyContainerTask DestroyTask;
CHI_END(Destroy)

CHI_BEGIN(Write)
struct WriteTask : public Task, TaskFlags<TF_SRL_SYM> {
	IN hipc::Pointer data_;
        IN size_t data_offset_;
        IN size_t data_size_;
        IN hipc::Pointer filename_;
        IN size_t filenamelen_;

	/** SHM default constructor */
	HSHM_INLINE explicit WriteTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : Task(alloc) {}

	/** Emplace constructor */
	HSHM_INLINE explicit WriteTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc, const TaskNode &task_node,
				       const PoolId &pool_id, const DomainQuery &dom_query,
				       const hipc::Pointer &data, size_t data_size, size_t data_offset, const hipc::Pointer &filename, size_t filenamelen) : Task(alloc) { // 
	// Initialize task
	task_node_ = task_node;
	prio_ = TaskPrioOpt::kHighLatency;
	pool_ = pool_id;
	method_ = Method::kWrite;
	task_flags_.SetBits(0);
	dom_query_ = dom_query;
	
	// Custom
	filename_ = filename;
	data_ = data;
	data_size_ = data_size;
	data_offset_ = data_offset;
	filename_ = filename;
	filenamelen_ = filenamelen;
	}
	
	/** Duplicate message */
	void CopyStart(const WriteTask &other, bool deep) {
	data_ = other.data_;
	data_size_ = other.data_size_;
        data_offset_ = other.data_offset_;
	filename_ = other.filename_;
	filenamelen_ = other.filenamelen_;
	if (!deep) {
		UnsetDataOwner();
	}
	}
					      
	/** (De)serialize message call */
	template <typename Ar>
	void SerializeStart(Ar &ar) {
	  ar.bulk(DT_WRITE, data_, data_size_);
	  ar.bulk(DT_WRITE, filename_, filenamelen_);
	  ar(data_size_, data_offset_);
	}
					      
	/** (De)serialize message return */
	template <typename Ar>
	void SerializeEnd(Ar &ar) {}
};
CHI_END(Write)

CHI_BEGIN(Read)
/** The ReadTask task */
struct ReadTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN hipc::Pointer data_;
  IN size_t data_offset_;
  IN size_t data_size_;

  /** SHM default constructor */
  HSHM_INLINE explicit
  ReadTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_INLINE explicit
  ReadTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
                const TaskNode &task_node,
                const PoolId &pool_id,
	   const DomainQuery &dom_query,
	   const hipc::Pointer &data, size_t data_size, size_t data_offset) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    prio_ = TaskPrioOpt::kLowLatency;
    pool_ = pool_id;
    method_ = Method::kRead;
    task_flags_.SetBits(0);
    dom_query_ = dom_query;

    // Custom
    data_ = data;
    data_size_ = data_size;
    data_offset_ = data_offset;
  }

  /** Duplicate message */
  void CopyStart(const ReadTask &other, bool deep) {
    data_ = other.data_;
    data_size_ = other.data_size_;
    data_offset_ = other.data_offset_;
    if (!deep) {
      UnsetDataOwner();
    }
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    ar.bulk(DT_WRITE, data_, data_size_);
    ar(data_size_, data_offset_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }
};
CHI_END(Read);

CHI_AUTOGEN_METHODS  // keep at class bottom

}  // namespace chi::dt_write

#endif  // CHI_TASKS_TASK_TEMPL_INCLUDE_dt_write_dt_write_TASKS_H_
