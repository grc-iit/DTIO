#ifndef DTIO_TASK_BUILDER_H
#define DTIO_TASK_BUILDER_H

#include <dtio/common/exceptions.h>
#include <chrono>
#include <dtio/common/client_interface/distributed_queue.h>
#include <dtio/common/data_structures.h>
#include <dtio/common/enumerations.h>
#include <memory>

class task_builder
{
protected:
  service service_i;

public:
  explicit task_builder(service service) : service_i(service) {}

  virtual std::vector<task *> build_write_task(task tsk, const char *data) {
    throw NotImplementedException("build_write_task");
  }

  virtual std::vector<task> build_read_task(task t) {
    throw NotImplementedException("build_read_task");
  }

  virtual std::vector<task> build_delete_task(task tsk) {
    throw NotImplementedException("build_delete_task");
  }

  virtual ~task_builder() {}
};

#endif // DTIO_TASK_BUILDER_H
