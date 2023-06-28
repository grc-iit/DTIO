/*
 * Copyright (C) 2023  SCS Lab <scs-help@cs.iit.edu>, Hariharan
 * Devarajan <hdevarajan@hawk.iit.edu>, Anthony Kougkas
 * <akougkas@iit.edu>, Xian-He Sun <sun@iit.edu>
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
/*******************************************************************************
 * Created by hariharan on 2/17/18.
 * Updated by akougkas on 6/26/2018
 * Updated by kbateman and nrajesh (nrj5k) since
 ******************************************************************************/
#ifndef DTIO_MAIN_STRUCTURE_H
#define DTIO_MAIN_STRUCTURE_H

#include "dtio/common/data_structures.h"
#include "hcl/common/macros.h"
#include <cereal/types/common.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <dtio/common/constants.h>
#include <dtio/common/enumerations.h>
#include <utility>
#include <vector>

#include <hcl.h>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#include <rpc/server.h>

struct HCLTaskKey
{
  size_t a;
  HCLTaskKey () : a (0) {}
  HCLTaskKey (size_t a_) : a (a_) {}
  HCLTaskKey (task a_) : a (std::hash<task>{}(a_)) {}
  MSGPACK_DEFINE (a);
  bool
  operator== (const HCLTaskKey &o) const
  {
    return a == o.a;
  }
  HCLTaskKey &
  operator= (const HCLTaskKey &other)
  {
    a = other.a;
    return *this;
  }
  bool
  operator< (const HCLTaskKey &o) const
  {
    return a < o.a;
  }
  bool
  operator> (const HCLTaskKey &o) const
  {
    return a > o.a;
  }
  bool
  Contains (const HCLTaskKey &o) const
  {
    return a == o.a;
  }
};

struct HCLKeyType
{
  size_t a;
  HCLKeyType () : a (0) {}
  HCLKeyType (size_t a_) : a (a_) {}
  HCLKeyType (std::string a_) : a (std::hash<std::string>{}(a_)) {}
  MSGPACK_DEFINE (a);
  bool
  operator== (const HCLKeyType &o) const
  {
    return a == o.a;
  }
  HCLKeyType &
  operator= (const HCLKeyType &other)
  {
    a = other.a;
    return *this;
  }
  bool
  operator< (const HCLKeyType &o) const
  {
    return a < o.a;
  }
  bool
  operator> (const HCLKeyType &o) const
  {
    return a > o.a;
  }
  bool
  Contains (const HCLKeyType &o) const
  {
    return a == o.a;
  }
};
namespace std
{
template <> struct hash<HCLKeyType>
{
  size_t
  operator() (const HCLKeyType &k) const
  {
    return k.a;
  }
};
}
typedef boost::interprocess::allocator<
    char, boost::interprocess::managed_mapped_file::segment_manager>
    CharAllocator;
typedef boost::interprocess::basic_string<char, std::char_traits<char>,
                                          CharAllocator>
    MappedUnitString;

/******************************************************************************
 *message_key structure
 ******************************************************************************/
struct message_key
{
  message_type m_type;
  map_type mp_type;
  operation operation_type;
  char key[KEY_SIZE];
};

struct message
{
  message_type m_type;
  map_type mp_type;
  char key[KEY_SIZE];
  size_t data_size;
  char *data;
};

struct file
{
  location_type location;
  std::string filename;
  int64_t offset;
  std::size_t size;
  int worker = -1;
  int server = -1;

  file (std::string filename, int64_t offset, std::size_t file_size)
      : filename (std::move (filename)), offset (offset), size (file_size),
        location (CACHE)
  {
  }
  file (const file &file_t)
      : filename (file_t.filename), offset (file_t.offset), size (file_t.size),
        location (file_t.location), worker (file_t.worker),
        server (file_t.server)
  {
  }
  file () : location (CACHE), filename (""), offset (0), size (0) {}

  virtual ~file () {}
  // serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->filename, this->offset, this->size, this->location,
             this->worker, this->server);
  }
};

struct chunk_meta
{
  file actual_user_chunk;
  file destination;
  // serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->actual_user_chunk, this->destination);
  }
};

// chunk_msg structure
struct chunk_msg
{
  location_type chunkType;
  std::string dataspace_id;
  std::string filename;
  size_t offset;
  size_t file_size;
};

// file_meta structure
struct file_meta
{
  file file_struct;
  std::vector<chunk_meta> chunks;

  file_meta () : chunks () {}

  virtual ~file_meta () {}
};

// dataspace structure
struct dataspace
{
  size_t size;
  void *data;
};

// file_stat structure
struct file_stat
{
  FILE *fh;
  std::size_t file_pointer;
  std::size_t file_size;
  std::string mode;
  bool is_open;

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->file_pointer, this->file_size, this->mode, this->is_open);
  }
};

// task structure
struct task
{
  task_type t_type;
  int64_t task_id;
  bool publish;
  bool addDataspace;
  bool async;

  explicit task (task_type t_type)
      : t_type (t_type), task_id (0), publish (false), addDataspace (true),
        async (true)
  {
  }
  task (const task &t_other)
      : t_type (t_other.t_type), task_id (t_other.task_id),
        publish (t_other.publish), addDataspace (t_other.addDataspace),
        async (t_other.async)
  {
  }

  virtual ~task () {}

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->t_type, this->task_id);
  }
};

namespace std
{
template <> struct hash<task>
{
  size_t
  operator() (const task &k) const
  {
    return k.task_id % HCL_CONF->NUM_SERVERS;
  }
  size_t
  operator() (const int64_t &task_id) const
  {
    return task_id % HCL_CONF->NUM_SERVERS;
  }
};
}

// write_task structure
struct write_task : public task
{
  file source;
  file destination;
  bool meta_updated = false;

  write_task () : task (task_type::WRITE_TASK) {}
  write_task (const file &source, const file &destination)
      : task (task_type::WRITE_TASK), source (source),
        destination (destination)
  {
  }
  write_task (const write_task &write_task_t)
      : task (task_type::WRITE_TASK), source (write_task_t.source),
        destination (write_task_t.destination)
  {
  }

  virtual ~write_task () {}

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->t_type, this->task_id, this->source, this->destination,
             this->meta_updated);
  }
};

// read_task structure
struct read_task : public task
{
  file source;
  file destination;
  bool meta_updated = false;
  bool local_copy = false;

  read_task (const file &source, const file &destination)
      : task (task_type::READ_TASK), source (source), destination (destination)
  {
  }
  read_task () : task (task_type::READ_TASK) {}
  read_task (const read_task &read_task_t)
      : task (task_type::READ_TASK), source (read_task_t.source),
        destination (read_task_t.destination)
  {
  }

  virtual ~read_task () {}

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->t_type, this->task_id, this->source, this->destination,
             this->meta_updated, this->local_copy);
  }
};

// flush_task structure
struct flush_task : public task
{
  file source;
  file destination;

  flush_task () : task (task_type::FLUSH_TASK) {}
  flush_task (const flush_task &flush_task_t)
      : task (task_type::FLUSH_TASK), source (flush_task_t.source),
        destination (flush_task_t.destination)
  {
  }
  flush_task (file source, file destination, location_type dest_t,
              std::string datasource_id)
      : task (task_type::FLUSH_TASK), source (source),
        destination (destination)
  {
  }

  virtual ~flush_task () {}

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->t_type, this->task_id, this->source, this->destination);
  }
};

// delete_task structure
struct delete_task : public task
{
  file source;

  delete_task () : task (task_type::DELETE_TASK) {}
  delete_task (const delete_task &delete_task_i)
      : task (task_type::DELETE_TASK), source (delete_task_i.source)
  {
  }
  explicit delete_task (file &source)
      : task (task_type::DELETE_TASK), source (source)
  {
  }

  virtual ~delete_task () {}

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->t_type, this->task_id, this->source);
  }
};

// solver_input structure
struct solver_input
{
  std::vector<task *> tasks;
  int num_tasks;
  int64_t *task_size;

  explicit solver_input (std::vector<task *> &task_list, int num_tasks)
  {
    this->tasks = task_list;
    this->num_tasks = num_tasks;
    task_size = new int64_t[num_tasks];
  }
  solver_input (const solver_input &other)
      : tasks (other.tasks), num_tasks (other.num_tasks),
        task_size (other.task_size)
  {
  }

  virtual ~solver_input () {}
};

// solver_output structure
struct solver_output
{
  int *solution;
  int num_task;
  // workerID->list of tasks
  std::unordered_map<int, std::vector<task *> > worker_task_map;

  explicit solver_output (int num_task)
      : worker_task_map (), num_task (num_task)
  {
    solution = new int[num_task];
  }
  solver_output (const solver_output &other)
      : worker_task_map (other.worker_task_map), num_task (other.num_task),
        solution (other.solution)
  {
  }

  virtual ~solver_output () {}
};
#endif // DTIO_MAIN_STRUCTURE_H
