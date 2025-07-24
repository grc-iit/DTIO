#ifndef CHI_DTIOMOD_METHODS_H_
#define CHI_DTIOMOD_METHODS_H_

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kWrite = 10;
  TASK_METHOD_T kRead = 11;
  TASK_METHOD_T kPrefetch = 12;
  TASK_METHOD_T kMetaPut = 13;
  TASK_METHOD_T kMetaGet = 14;
  TASK_METHOD_T kSchedule = 15;
  TASK_METHOD_T kCount = 16;
};

#endif  // CHI_DTIOMOD_METHODS_H_
