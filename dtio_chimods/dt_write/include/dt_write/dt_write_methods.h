#ifndef CHI_DT_WRITE_METHODS_H_
#define CHI_DT_WRITE_METHODS_H_

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kWrite = 10;
  TASK_METHOD_T kRead = 11;
  TASK_METHOD_T kCount = 12;
};

#endif  // CHI_DT_WRITE_METHODS_H_