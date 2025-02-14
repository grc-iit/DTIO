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
#ifndef DTIO_MAIN_STRUCTURE_H
#define DTIO_MAIN_STRUCTURE_H

// #include <hcl/common/macros.h>
#include <cereal/types/common.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <dtio/common/constants.h>
#include <dtio/common/enumerations.h>
// #include <rpc/msgpack/adaptor/define_decl.hpp>
#include <utility>
#include <vector>

#ifndef HCL_COMMUNICATION_ENABLE_THALLIUM
#define HCL_COMMUNICATION_ENABLE_THALLIUM 1
#endif

#include <hcl.h>
// #include <hcl/common/data_structures.h>
// #include <rpc/client.h>
// #include <rpc/rpc_error.h>
// #include <rpc/server.h>

#include <string.h>

// This exists so that we can make the buffer larger
typedef struct DTIOCharStruct {
  size_t length;
  char value[MAX_IO_UNIT];

  DTIOCharStruct() {}
  DTIOCharStruct(const DTIOCharStruct &other)
    : DTIOCharStruct(other.value, other.length) {} /* copy constructor*/
  DTIOCharStruct(DTIOCharStruct &&other)
    : DTIOCharStruct(other.value, other.length) {} /* move constructor*/

  DTIOCharStruct(const char *data_) {
    snprintf(this->value, MAX_IO_UNIT, "%s", data_);
  }
  DTIOCharStruct(std::string data_) : DTIOCharStruct(data_.c_str(), data_.size()) {}

  DTIOCharStruct(char *data_, size_t size) {
    snprintf(this->value, size + 1, "%s", data_);
    this->length = size;
  }

  DTIOCharStruct(const char *data_, size_t size) {
    snprintf(this->value, size + 1, "%s", data_);
    this->length = size;
  }

  // MSGPACK_DEFINE(length, value);
  
  void Set(char *data_, size_t size) {
    snprintf(this->value, size, "%s", data_);

    this->length = size;
  }
  void Set(std::string data_) {
    snprintf(this->value, data_.size(), "%s", data_.c_str());
    this->length = data_.size();
  }

  const char *c_str() const { return this->value; }
  std::string string() const { return std::string(this->value, this->length); }

  char *data() { return value; }
  const size_t size() const { return this->length; }
  /**
   * Operators
   */
  DTIOCharStruct &operator=(const DTIOCharStruct &other) {
    strncpy(this->value, other.value, other.length);
    this->length = other.length;
    return *this;
  }
  /* equal operator for comparing two Chars. */
  bool operator==(const DTIOCharStruct &o) const {
    return (this->length == o.length) && (strncmp(value, o.value, this->length) == 0);
  }
  DTIOCharStruct operator+(const DTIOCharStruct &o) {
    std::string added = std::string(this->value, this->length) + std::string(o.value, o.length);
    return DTIOCharStruct(added);
  }
  DTIOCharStruct operator+(std::string &o) {
    std::string added = std::string(this->value, this->length) + o;
    return DTIOCharStruct(added);
  }
  DTIOCharStruct &operator+=(const DTIOCharStruct &rhs) {
    std::string added = std::string(this->c_str()) + std::string(rhs.c_str());
    Set(added);
    return *this;
  }
  bool operator>(const DTIOCharStruct &o) const {
    return this->length == o.length && strncmp(this->value, o.value, this->length) > 0;
  }
  bool operator>=(const DTIOCharStruct &o) const {
    return this->length == o.length && strncmp(this->value, o.value, this->length) >= 0;
  }
  bool operator<(const DTIOCharStruct &o) const {
    return this->length == o.length && strcmp(this->value, o.value) < 0;
  }
  bool operator<=(const DTIOCharStruct &o) const {
    return this->length == o.length && strcmp(this->value, o.value) <= 0;
  }


  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->length, cereal::binary_data(this->value, MAX_IO_UNIT));
  }

} DTIOCharStruct;

static DTIOCharStruct operator+(const std::string &a1, const DTIOCharStruct &a2) {
  std::string added = a1 + std::string(a2.value, a2.length + 1);
  return DTIOCharStruct(added);
}

namespace std {
template <>
struct hash<DTIOCharStruct> {
  size_t operator()(const DTIOCharStruct &k) const {
    std::string val(k.value, k.length + 1);
    return std::hash<std::string>()(val);
  }
};
}  // namespace std

struct HCLKeyType
{
  size_t a;
  HCLKeyType () : a (0) {}
  HCLKeyType (size_t a_) : a (a_) {}
  HCLKeyType (std::string a_) : a (std::hash<std::string>()(a_)) {}
  // MSGPACK_DEFINE (a);
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
    // return 0;
    return k.a;
  }
};
}

template <typename A>
void serialize(A &ar, HCLKeyType &a) {
  ar &a.a;
}

typedef boost::interprocess::allocator<char, boost::interprocess::managed_mapped_file::segment_manager> CharAllocator;
typedef bip::basic_string<char, std::char_traits<char>, CharAllocator> MappedUnitString;

// typedef boost::interprocess::allocator<char, boost::interprocess::managed_mapped_file::segment_manager> CharAllocator;
// typedef bip::basic_string<char, std::char_traits<char>, CharAllocator> MappedUnitString;

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
  char filename[DTIO_FILENAME_MAX];
  int64_t offset;
  std::size_t size;
  int worker;
  int server;

  file (std::string filename_, int64_t offset, std::size_t file_size)
    : filename (), offset (offset), size (file_size),
      location (CACHE), worker(-1), server(-1)
  {
    strncpy(filename, filename_.c_str(), DTIO_FILENAME_MAX);
  }

  file (const file &file_t)
    : filename (), offset (file_t.offset), size (file_t.size),
        location (file_t.location), worker (file_t.worker),
        server (file_t.server)
  {
    strncpy(filename, file_t.filename, DTIO_FILENAME_MAX);
  }
  file () : location (CACHE), filename (""), offset (0), size (0), worker(-1), server(-1) {}

  // MSGPACK_DEFINE(location, filename, offset, size, worker, server);

  ~file () {}

  file &
  operator= (const file &other)
  {
    location = other.location;
    strncpy(filename, other.filename, DTIO_FILENAME_MAX);
    offset = other.offset;
    size = other.size;
    worker = other.worker;
    server = other.server;
    return *this;
  }

  // serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    auto location_ = static_cast<int>(this->location);
    archive (cereal::binary_data(this->filename, DTIO_FILENAME_MAX), this->offset, this->size, location_,
             this->worker, this->server);
    location = static_cast<location_type>(location_);
  }
};

// MSGPACK_ADD_ENUM(location_type);

namespace std
{
template <> struct hash<file>
{
  size_t
  operator() (const file *k) const
  {
    std::string val(k->filename);
    return std::hash<std::string>()(val);
  }
  size_t
  operator() (const file &k) const
  {
    std::string val(k.filename);
    return std::hash<std::string>()(val);
  }
};
}

// The idea behind this was to remove the filename from chunk metadata so that
// we can store more of them, but it needs to be changed to store IDs, along
// with files, so we're just decreasing max filename size for now.

// struct chunk
// {
//   location_type location;
//   int64_t offset;
//   std::size_t size;
//   int worker;
//   int server;

//   chunk (int64_t offset, std::size_t file_size)
//     : offset (offset), size (file_size),
//       location (CACHE), worker(-1), server(-1) {}

//   chunk (const chunk &chunk_t)
//     : offset (chunk_t.offset), size (chunk_t.size),
//         location (chunk_t.location), worker (chunk_t.worker),
//         server (chunk_t.server) {}
//   chunk () : location (CACHE), offset (0), size (0), worker(-1), server(-1) {}

//   // MSGPACK_DEFINE(location, filename, offset, size, worker, server);

//   ~chunk () {}

//   chunk &
//   operator= (const chunk &other)
//   {
//     location = other.location;
//     offset = other.offset;
//     size = other.size;
//     worker = other.worker;
//     server = other.server;
//     return *this;
//   }

//   // serialization
//   template <class Archive>
//   void
//   serialize (Archive &archive)
//   {
//     archive (this->offset, this->size, this->location,
//              this->worker, this->server);
//   }
// };

struct chunk_meta
{
  file actual_user_chunk;
  file destination;

  // MSGPACK_DEFINE(actual_user_chunk, destination);

  chunk_meta &
  operator= (const chunk_meta &other)
  {
    actual_user_chunk = other.actual_user_chunk;
    destination = other.destination;
    return *this;
  }
 
  // serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->actual_user_chunk, this->destination);
  }
};


namespace std
{
template <> struct hash<chunk_meta>
{
  size_t
  operator() (const chunk_meta *k) const
  {
    return std::hash<file>()(k->actual_user_chunk) ^ std::hash<file>()(k->destination);
  }
  size_t
  operator() (const chunk_meta &k) const
  {
    return std::hash<file>()(k.actual_user_chunk) ^ std::hash<file>()(k.destination);
  }
};
}

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
  std::array<chunk_meta, CHUNK_LIMIT> chunks;
  int current_chunk_index;
  int num_chunks;

  file_meta () : chunks (), current_chunk_index(0), num_chunks(0) {}
  file_meta(const file_meta &other)
    : file_meta(other.file_struct, other.chunks, other.current_chunk_index, other.num_chunks) {} /* copy constructor*/
  file_meta(file_meta &&other)
    : file_meta(other.file_struct, other.chunks, other.current_chunk_index, other.num_chunks) {} /* move constructor*/

  // file_meta(file file_struct_, const std::array<chunk_meta, CHUNK_LIMIT> chunks_, int current_chunk_index_, int num_chunks_) {
  //   this->file_struct = file_struct_;
  //   this->current_chunk_index = current_chunk_index_;
  //   std::cout << "Copying chunks in constructor from current index " << current_chunk_index << std::endl;
  //   for (int i = 0; i < num_chunks_; i++) {
  //     if (current_chunk_index_ - i - 1 >= 0) {
  // 	this->chunks[current_chunk_index_ - i - 1] = chunks_[current_chunk_index_ - i - 1];
  //     }
  //     else {
  // 	this->chunks[CHUNK_LIMIT + current_chunk_index_ - i - 1] = chunks_[CHUNK_LIMIT + current_chunk_index_ - i - 1];
  //     }
  //   }
  //   this->num_chunks = num_chunks_;
  // }

  file_meta(file file_struct_, std::array<chunk_meta, CHUNK_LIMIT> chunks_, int current_chunk_index_, int num_chunks_) {
    this->file_struct = file_struct_;
    this->current_chunk_index = current_chunk_index_;
    this->num_chunks = num_chunks_;
    std::cout << "Copying chunks in constructor from current index " << current_chunk_index << std::endl;
    for (int i = 0; i < num_chunks_; i++) {
      if (current_chunk_index_ - i - 1 >= 0) {
	this->chunks[current_chunk_index_ - i - 1] = chunks_[current_chunk_index_ - i - 1];
      }
      else {
	this->chunks[CHUNK_LIMIT + current_chunk_index_ - i - 1] = chunks_[CHUNK_LIMIT + current_chunk_index_ - i - 1];
      }
    }
  }

  void append (chunk_meta *cm) {
    chunks[current_chunk_index] = *cm;
    ++current_chunk_index;
    if (num_chunks < CHUNK_LIMIT) {
      ++num_chunks;
    }
    if (current_chunk_index >= CHUNK_LIMIT) {
      current_chunk_index = 0;
    }
  } 

  file_meta &
  operator= (const file_meta &other)
  {
    file_struct = other.file_struct;
    current_chunk_index = other.current_chunk_index;
    num_chunks = other.num_chunks;
    std::cout << "Copying chunks from current index " << current_chunk_index << std::endl;
    for (int i = 0; i < num_chunks; i++) {
      if (current_chunk_index - i - 1 >= 0) {
	std::cout << "From start" << std::endl;
	chunks[current_chunk_index - i - 1] = other.chunks[current_chunk_index - i - 1];
      }
      else {
	std::cout << "From end" << std::endl;
	chunks[CHUNK_LIMIT + current_chunk_index - i - 1] = other.chunks[CHUNK_LIMIT + current_chunk_index - i - 1];
      }
    }
    std::cout << "Finished copying chunks" << std::endl;
    return *this;
  }

  // serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->file_struct, this->chunks, this->current_chunk_index, this->num_chunks);
  }

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
  int fd; // Adding this to file stat could be expensive, should it replace fh?
  std::size_t file_pointer;
  std::size_t file_size;
  int flags;
  mode_t posix_mode;
  std::string mode;
  bool is_open;

  file_stat &
  operator= (const file_stat &other)
  {
    fh = other.fh;
    fd = other.fd;
    file_pointer = other.file_pointer;
    file_size = other.file_size;
    flags = other.flags;
    posix_mode = other.posix_mode;
    mode = other.mode;
    is_open = other.is_open;
    return *this;
  }

  // MSGPACK_DEFINE(fd, file_pointer, file_size, flags, posix_mode, mode, is_open);

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    archive (this->fd, this->file_pointer, this->file_size, this->flags, this->posix_mode, this->mode, this->is_open);
  }
};

namespace std
{
template <> struct hash<file_stat>
{
  size_t
  operator() (const file_stat *k) const
  {
    if (k->fh == NULL) {
      return k->fd;
    }
    else {
      return (size_t)k->fh;
    }
  }
  size_t
  operator() (const file_stat &k) const
  {
    if (k.fh == NULL) {
      return k.fd;
    }
    else {
      return (size_t)k.fh;
    }
  }
};
}


// task structure
struct task
{
  task_type t_type;
  io_client_type iface;
  int64_t task_id;
  bool publish;
  bool addDataspace;
  bool async;

  /* Task-specific info
   * ******************
   * read_task needs all of them
   * write_task needs source, dest, meta_updated
   * flush_task needs source, dest
   * delete_task needs source
   */
  file source;
  file destination;
  bool meta_updated = false;
  bool local_copy = false;
  bool check_fs = false;

  task ()
    : t_type (task_type::READ_TASK), task_id (0), publish (true),
      iface(io_client_type::POSIX), addDataspace (true), async (true), source (),
      destination (), meta_updated(false), local_copy(false), check_fs(false)
  {
  }

  // MSGPACK_DEFINE(t_type, iface, task_id, publish, addDataspace, async, source, destination, meta_updated, local_copy, check_fs);

  task (task_type t_type)
    : t_type (t_type), iface (io_client_type::POSIX), task_id (0), publish (true), addDataspace (true),
      async (true), source (), destination (), meta_updated(false), local_copy(false), check_fs(false)
  {
  }
  task (const task &t_other)
    : t_type (t_other.t_type), iface (t_other.iface), task_id (t_other.task_id),
        publish (t_other.publish), addDataspace (t_other.addDataspace),
      async (t_other.async), source (t_other.source), destination(t_other.destination),
      meta_updated(t_other.meta_updated), local_copy(t_other.local_copy), check_fs(t_other.check_fs)
  {
  }

  task(task_type t_type_, const file &source_, const file &destination_) : t_type(t_type_), source(source_), destination(destination_), task_id (0), iface(io_client_type::POSIX), publish (true),
      addDataspace (true), async (true), meta_updated(false), local_copy(false), check_fs(false)
  {
  }
  task (task_type t_type_, file &source)
    : t_type (t_type_), source (source), task_id (0), publish (true),
      iface(io_client_type::POSIX), addDataspace (true), async (true),
      destination (), meta_updated(false), local_copy(false), check_fs(false)
  {
  }

  ~task () {}

  bool
  operator== (const task &o) const
  {
    return task_id == o.task_id;
  }

  task &
  operator= (const task &other)
  {
    t_type = other.t_type;
    iface = other.iface;
    task_id = other.task_id;
    publish = other.publish;
    addDataspace = other.addDataspace;
    async = other.async;
    source = other.source;
    destination = other.destination;
    return *this;
  }

  bool
  operator< (const task &o) const
  {
    return task_id < o.task_id;
  }
  bool
  operator> (const task &o) const
  {
    return task_id > o.task_id;
  }
  bool
  Contains (const task &o) const
  {
    return task_id == o.task_id;
  }

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    auto t_type_ = static_cast<int64_t>(this->t_type);
    auto iface_ = static_cast<int>(this->iface);
    archive (t_type_, iface_, this->task_id, this->publish, this->addDataspace, this->async, this->source, this->destination, this->meta_updated, this->local_copy, this->check_fs);
    t_type = static_cast<task_type>(t_type_);
    iface = static_cast<io_client_type>(iface_);
  }
};

// MSGPACK_ADD_ENUM(task_type);
// MSGPACK_ADD_ENUM(io_client_type);

namespace std
{
template <> struct hash<task>
{
  size_t
  operator() (const task *k) const
  {
    return k->task_id;
  }
  size_t
  operator() (const task &k) const
  {
    return k.task_id;
  }
  size_t
  operator() (const int64_t &task_id) const
  {
    return task_id;
  }
};
}

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
