//
// Created by lukemartinlogan on 8/11/23.
//

#ifndef CHI_TASKS_TASK_TEMPL_INCLUDE_dtiomod_dtiomod_TASKS_H_
#define CHI_TASKS_TASK_TEMPL_INCLUDE_dtiomod_dtiomod_TASKS_H_

#include "chimaera/chimaera_namespace.h"
#include "enumerations.h"

namespace chi::dtiomod {

#include "dtiomod_methods.h"
CHI_NAMESPACE_INIT

CHI_BEGIN(Create)
/** A task to create dtiomod */
struct CreateTaskParams {
  CLS_CONST char *lib_name_ = "example_dtiomod";
  int dtiomod_id_;

  HSHM_INLINE_CROSS_FUN
  CreateTaskParams() = default;

  HSHM_INLINE_CROSS_FUN
  CreateTaskParams(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
		  int dtiomod_id = 0) {
	  dtiomod_id_ = dtiomod_id;
  }

  template <typename Ar>
  HSHM_INLINE_CROSS_FUN void serialize(Ar &ar) {
    ar(dtiomod_id_);
  }
};
typedef chi::Admin::CreatePoolBaseTask<CreateTaskParams> CreateTask;
CHI_END(Create)

CHI_BEGIN(Destroy)
/** A task to destroy dtiomod */
typedef chi::Admin::DestroyContainerTask DestroyTask;
CHI_END(Destroy)

CHI_BEGIN(Write)
struct WriteTask : public Task, TaskFlags<TF_SRL_SYM> {
	IN hipc::Pointer data_;
        IN size_t data_offset_;
        IN size_t data_size_;
        IN hipc::Pointer filename_;
        IN size_t filenamelen_;
        IN io_client_type iface_;

	/** SHM default constructor */
	HSHM_INLINE explicit WriteTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : Task(alloc) {}

	/** Emplace constructor */
	HSHM_INLINE explicit WriteTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc, const TaskNode &task_node,
				       const PoolId &pool_id, const DomainQuery &dom_query,
				       const hipc::Pointer &data, size_t data_size, size_t data_offset, const hipc::Pointer &filename, size_t filenamelen, io_client_type iface) : Task(alloc) { // 
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
	iface_ = iface;
	}
	
	/** Duplicate message */
	void CopyStart(const WriteTask &other, bool deep) {
	data_ = other.data_;
	data_size_ = other.data_size_;
        data_offset_ = other.data_offset_;
	filename_ = other.filename_;
	filenamelen_ = other.filenamelen_;
	iface_ = other.iface_;
	if (!deep) {
		UnsetDataOwner();
	}
	}
					      
	/** (De)serialize message call */
	template <typename Ar>
	void SerializeStart(Ar &ar) {
	  ar.bulk(DT_WRITE, data_, data_size_);
	  ar.bulk(DT_WRITE, filename_, filenamelen_);
	  ar(data_size_, data_offset_, iface_);
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
  IN hipc::Pointer filename_;
  IN size_t filenamelen_;
  IN io_client_type iface_;

  /** SHM default constructor */
  HSHM_INLINE explicit
  ReadTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_INLINE explicit
  ReadTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
                const TaskNode &task_node,
                const PoolId &pool_id,
	   const DomainQuery &dom_query,
	   const hipc::Pointer &data, size_t data_size, size_t data_offset, const hipc::Pointer &filename, size_t filenamelen, io_client_type iface) : Task(alloc) {
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
    filename_ = filename;
    filenamelen_ = filenamelen;
    iface_ = iface;
  }

  /** Duplicate message */
  void CopyStart(const ReadTask &other, bool deep) {
    data_ = other.data_;
    data_size_ = other.data_size_;
    data_offset_ = other.data_offset_;
    filename_ = other.filename_;
    filenamelen_ = other.filenamelen_;
    iface_ = other.iface_;
    if (!deep) {
      UnsetDataOwner();
    }
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    ar.bulk(DT_WRITE, data_, data_size_);
    ar.bulk(DT_WRITE, filename_, filenamelen_);
    ar(data_size_, data_offset_, iface_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }
};
CHI_END(Read);

CHI_BEGIN(Prefetch)
/** The PrefetchTask task */
struct PrefetchTask : public Task, TaskFlags<TF_SRL_SYM> {
  /** SHM default constructor */
  HSHM_INLINE explicit
  PrefetchTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_INLINE explicit
  PrefetchTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
                const TaskNode &task_node,
                const PoolId &pool_id,
                const DomainQuery &dom_query) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    prio_ = TaskPrioOpt::kLowLatency;
    pool_ = pool_id;
    method_ = Method::kPrefetch;
    task_flags_.SetBits(0);
    dom_query_ = dom_query;

    // Custom
  }

  /** Duplicate message */
  void CopyStart(const PrefetchTask &other, bool deep) {
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }
};
CHI_END(Prefetch);

CHI_BEGIN(MetaPut)
/** The MetaPutTask task */
struct MetaPutTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN hipc::Pointer key_;
  IN size_t keylen_;
  IN hipc::Pointer val_;
  IN size_t vallen_;

  /** SHM default constructor */
  HSHM_INLINE explicit
  MetaPutTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_INLINE explicit
  MetaPutTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
                const TaskNode &task_node,
                const PoolId &pool_id,
	      const DomainQuery &dom_query,
	      hipc::Pointer key, size_t keylen,
	      hipc::Pointer val, size_t vallen) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    prio_ = TaskPrioOpt::kLowLatency;
    pool_ = pool_id;
    method_ = Method::kMetaPut;
    task_flags_.SetBits(0);
    dom_query_ = dom_query;

    // Custom
    key_ = key;
    val_ = val;
    keylen_ = keylen;
    vallen_ = vallen;
  }

  /** Duplicate message */
  void CopyStart(const MetaPutTask &other, bool deep) {
    key_ = other.key_;
    keylen_ = other.keylen_;
    val_ = other.val_;
    vallen_ = other.vallen_;
    if (!deep) {
      UnsetDataOwner();
    }
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    ar.bulk(DT_WRITE, key_, keylen_);
    ar.bulk(DT_WRITE, val_, vallen_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }
};
CHI_END(MetaPut);

CHI_BEGIN(MetaGet)
/** The MetaGetTask task */
struct MetaGetTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN hipc::Pointer key_;
  IN size_t keylen_;
  OUT hipc::Pointer val_;
  OUT size_t vallen_;
  OUT bool presence_;

  /** SHM default constructor */
  HSHM_INLINE explicit
  MetaGetTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_INLINE explicit
  MetaGetTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
                const TaskNode &task_node,
                const PoolId &pool_id,
	      const DomainQuery &dom_query,
	      hipc::Pointer key, size_t keylen) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    prio_ = TaskPrioOpt::kLowLatency;
    pool_ = pool_id;
    method_ = Method::kMetaGet;
    task_flags_.SetBits(0);
    dom_query_ = dom_query;

    // Custom
    key_ = key;
    keylen_ = keylen;
  }

  /** Duplicate message */
  void CopyStart(const MetaGetTask &other, bool deep) {
    key_ = other.key_;
    keylen_ = other.keylen_;
    presence_ = other.presence_;
    if (!deep) {
      UnsetDataOwner();
    }
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    ar.bulk(DT_WRITE, key_, keylen_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar.bulk(DT_WRITE, val_, vallen_);
    ar(presence_);
  }
};
CHI_END(MetaGet);

CHI_BEGIN(Schedule)
/** The ScheduleTask task */
struct ScheduleTask : public Task, TaskFlags<TF_SRL_SYM> {
  OUT size_t schedule_num_;

  /** SHM default constructor */
  HSHM_INLINE explicit
  ScheduleTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_INLINE explicit
  ScheduleTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
                const TaskNode &task_node,
                const PoolId &pool_id,
	       const DomainQuery &dom_query) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    prio_ = TaskPrioOpt::kLowLatency;
    pool_ = pool_id;
    method_ = Method::kSchedule;
    task_flags_.SetBits(0);
    dom_query_ = dom_query;

    // Custom
  }

  /** Duplicate message */
  void CopyStart(const ScheduleTask &other, bool deep) {
    schedule_num_ = other.schedule_num_;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(schedule_num_);
  }
};
CHI_END(Schedule);

CHI_AUTOGEN_METHODS  // keep at class bottom

}  // namespace chi::dtiomod

#endif  // CHI_TASKS_TASK_TEMPL_INCLUDE_dtiomod_dtiomod_TASKS_H_
