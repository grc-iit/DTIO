/*
 * Copyright (C) 2024 Gnosis Research Center <grc@iit.edu>, 
 * Keith Bateman <kbateman@hawk.iit.edu>, Neeraj Rajesh
 * <nrajesh@hawk.iit.edu> Hariharan Devarajan
 * <hdevarajan@hawk.iit.edu>, Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of DTIO
 *
 * DTIO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef DTIO_MAIN_DISTRIBUTEDQUEUE_H
#define DTIO_MAIN_DISTRIBUTEDQUEUE_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <dtio/common/data_structures.h>
#include <dtio/common/enumerations.h>
#include <dtio/common/exceptions.h>
#include <memory>

/******************************************************************************
 *Class
 ******************************************************************************/
class distributed_queue {
protected:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  service service_i;
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  explicit distributed_queue(service service) : service_i(service) {}

public:
  /******************************************************************************
   *Interface
   ******************************************************************************/
  virtual void clear() {
    throw NotImplementedException("clear");
  }

  virtual int publish_task(task *task_t) {
    throw NotImplementedException("publish_task");
  }
  virtual task *subscribe_task_with_timeout(int &status) {
    throw NotImplementedException("subscribe_task_with_timeout");
  }

  virtual task *subscribe_task(int &status) {
    throw NotImplementedException("subscribe_task");
  }

  virtual int get_queue_size() {
    throw NotImplementedException("get_queue_size");
  }
  virtual int get_queue_count() {
    throw NotImplementedException("get_queue_count");
  }
  virtual int get_queue_count_limit() {
    throw NotImplementedException("get_queue_count");
  }
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~distributed_queue() {}
};

#endif // DTIO_MAIN_DISTRIBUTEDQUEUE_H
