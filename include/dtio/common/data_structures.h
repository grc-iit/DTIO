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
#include <cereal/types/array.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <dtio/common/constants.h>
#include <dtio/common/enumerations.h>
#include <dtio/common/logger.h>
// #include <rpc/msgpack/adaptor/define_decl.hpp>
#include <utility>
#include <vector>

#ifndef HCL_COMMUNICATION_ENABLE_THALLIUM
#define HCL_COMMUNICATION_ENABLE_THALLIUM 1
#endif

// #include <rpc/client.h>
// #include <rpc/rpc_error.h>
// #include <rpc/server.h>

#include <string.h>
#include <regex>

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
  // std::ostream& operator<<(std::ostream& os, file const& arg)
  // {
  //   os << "location = " << arg.location << ", filename = " <<
  //   arg.filename << ", offset = " << arg.offset << ", size = " <<
  //   arg.size << ", worker = " << arg.worker << ", server = " <<
  //   arg.server;
  //   return os;
  // }

  // file from_string(const std::string& str) {
  //   std::regex pattern(R"(location = (\d+), filename = ([^,]+), )"
  //                     R"(offset = (-?\d+), size = (\d+), )"
  //                     R"(worker = (-?\d+), server = (-?\d+))");
    
  //   std::smatch matches;
  //   file result;

  //   if (std::regex_search(str, matches, pattern)) {
  //       // Parse location
  //       result.location = static_cast<location_type>(std::stoi(matches[1].str()));
        
  //       // Parse filename (with safety check for buffer size)
  //       std::strncpy(result.filename, matches[2].str().c_str(), 
  //                   DTIO_FILENAME_MAX - 1);
  //       result.filename[DTIO_FILENAME_MAX - 1] = '\0';  // Ensure null termination
        
  //       // Parse remaining numeric fields
  //       result.offset = std::stoll(matches[3].str());
  //       result.size = std::stoull(matches[4].str());
  //       result.worker = std::stoi(matches[5].str());
  //       result.server = std::stoi(matches[6].str());
  //   }

  //   return result;
  // }

  // std::string to_string(file const& arg)
  // {
  //   std::ostringstream ss;
  //   ss << arg;
  //   return std::move(ss).str();  // enable efficiencies in c++17
  // }

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

  // std::ostream& operator<<(std::ostream& os, chunk_meta const& arg)
  // {
  //   os << "actual_user_chunk = { " << arg.actual_user_chunk << " }, destination = { " << arg.destination << " }";
  //   return os;
  // }

  // chunk_meta from_string(const std::string& str) {
  //   chunk_meta result;
    
  //   // Regex pattern that matches the outer structure and captures the inner contents
  //   std::regex pattern(R"(actual_user_chunk = \{ (.*?) \}, destination = \{ (.*?) \})");
    
  //   std::smatch matches;
  //   if (std::regex_search(str, matches, pattern)) {
  //       // Extract the inner strings for each file struct
  //       std::string actual_user_chunk_str = matches[1].str();
  //       std::string destination_str = matches[2].str();
        
  //       // Use the previously defined parse_file_regex function to parse each file struct
  //       result.actual_user_chunk = result.actual_user_chunk.from_string(actual_user_chunk_str);
  //       result.destination = result.destination.from_string(destination_str);
  //   } else {
  //       throw std::invalid_argument("Invalid chunk_meta string format");
  //   }
    
  //   return result;
  // }

  // chunk_meta(const std::string& str) {
  //   *this = from_string(str);
  // }

  // std::string to_string(chunk_meta const& arg)
  // {
  //   std::ostringstream ss;
  //   ss << arg;
  //   return std::move(ss).str();  // enable efficiencies in c++17
  // }

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
    DTIO_LOG_DEBUG("Copying chunks in constructor from current index " << current_chunk_index);
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
    DTIO_LOG_DEBUG("Copying chunks from current index " << current_chunk_index);
    for (int i = 0; i < num_chunks; i++) {
      if (current_chunk_index - i - 1 >= 0) {
	chunks[current_chunk_index - i - 1] = other.chunks[current_chunk_index - i - 1];
      }
      else {
	chunks[CHUNK_LIMIT + current_chunk_index - i - 1] = other.chunks[CHUNK_LIMIT + current_chunk_index - i - 1];
      }
    }
    return *this;
  }

  // std::ostream& operator<<(std::ostream& os, file_meta const& arg)
  // {
  //   os << "file_struct = { " << arg.file_struct << " }, chunks = [ ";
  //   for (int i = 0; i < CHUNK_LIMIT; i++) {
  //     os << "{ " << arg.chunks[i] << " }, ";
  //   }
  //   os << " ], current_chunk_index = " << arg.current_chunk_index << ", num_chunks = " << arg.num_chunks;
  //   return os;
  // }

  // file_meta from_string(const std::string& str) {
  //     file_meta result;
    
  //     // First, match the overall structure
  //     std::regex main_pattern(R"(file_struct = \{ (.*?) \}, chunks = \[ (.*?) \], )"
  // 			      R"(current_chunk_index = (-?\d+), num_chunks = (-?\d+))");
    
  //     std::smatch main_matches;
  //     if (!std::regex_search(str, main_matches, pattern)) {
  //       throw std::invalid_argument("Invalid file_meta format");
  //     }

  //     try {
  //       // Parse file_struct
  //       result.file_struct = result.file_struct.from_string(main_matches[1].str());
        
  //       // Parse chunks array
  //       std::string chunks_str = main_matches[2].str();
        
  //       // Pattern for individual chunk entries
  //       std::regex chunk_pattern(R"(\{ (.*?) \})");
        
  //       // Iterator for finding all chunks
  //       std::string::const_iterator searchStart(chunks_str.cbegin());
  //       std::string::const_iterator searchEnd(chunks_str.cend());
  //       std::smatch chunk_matches;
        
  //       int chunk_index = 0;
  //       while (std::regex_search(searchStart, searchEnd, chunk_matches, chunk_pattern) 
  //              && chunk_index < CHUNK_LIMIT) {
  //           // Parse each chunk_meta
  //           result.chunks[chunk_index] = result.chunks[0].from_string(chunk_matches[1].str());
  //           chunk_index++;
            
  //           // Move to next chunk
  //           searchStart = chunk_matches.suffix().first;
  //       }
        
  //       // Parse the remaining simple integers
  //       result.current_chunk_index = std::stoi(main_matches[3].str());
  //       result.num_chunks = std::stoi(main_matches[4].str());
        
  //     } catch (const std::exception& e) {
  //       throw std::invalid_argument(std::string("Failed to parse file_meta: ") + e.what());
  //     }
    
  //     return result;
  // }

  // file_meta(const std::string& str) {
  //   *this = from_string(str);
  // }

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

    // Default constructor (required for serialization)
    file_stat() 
        : fh(nullptr)
        , fd(-1)
        , file_pointer(0)
        , file_size(0)
        , flags(0)
        , posix_mode(0)
        , mode("")
        , is_open(false) {}

  file_stat(FILE *fh_, int fd_, std::size_t file_pointer_, std::size_t file_size_,
	    int flags_, mode_t posix_mode_, std::string mode_, bool is_open_) 
        : fh(fh_)
        , fd(fd_)
        , file_pointer(file_pointer_)
        , file_size(file_size_)
        , flags(flags_)
        , posix_mode(posix_mode_)
        , mode(mode_)
        , is_open(is_open_) {}

    // Copy constructor
    file_stat(const file_stat& other)
        : fh(other.fh)
        , fd(other.fd)
        , file_pointer(other.file_pointer)
        , file_size(other.file_size)
        , flags(other.flags)
        , posix_mode(other.posix_mode)
        , mode(other.mode)
        , is_open(other.is_open) {}

    // Move constructor
    file_stat(file_stat&& other) noexcept
        : fh(std::exchange(other.fh, nullptr))
        , fd(std::exchange(other.fd, -1))
        , file_pointer(std::exchange(other.file_pointer, 0))
        , file_size(std::exchange(other.file_size, 0))
        , flags(std::exchange(other.flags, 0))
        , posix_mode(std::exchange(other.posix_mode, 0))
        , mode(std::move(other.mode))
        , is_open(std::exchange(other.is_open, false)) {}

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

  // std::ostream& operator<<(std::ostream& os, file_stat const& arg)
  // {
  //   os << "fh = " << arg.fh << ", fd = " << arg.fd << ", file_pointer = " << arg.file_pointer <<
  //     ", file_size = " << arg.file_size << ", flags = " << arg.flags << ", arg.posix_mode = " << arg.posix_mode <<
  //     ", is_open = " << arg.is_open;
  //   return os;
  // }

  // file_stat from_string(const std::string& str) {
  //   std::regex pattern(R"(fh = (.*?), fd = (\d+), file_pointer = (\d+), )"
  // 		       R"(file_size = (\d+), flags = (\d+), )"
  // 		       R"(arg\.posix_mode = (\d+), is_open = (\d+))");
  //   std::smatch matches;
  //   if (std::regex_search(str, matches, pattern)) {
  //     // Convert hex string to pointer for fh
  //     std::istringstream ss(matches[1].str());
  //     ss >> std::hex >> result.fh;

  //     result.fd = std::stoi(matches[2].str());
  //     result.file_pointer = std::stoull(matches[3].str());
  //     result.file_size = std::stoull(matches[4].str());
  //     result.flags = std::stoi(matches[5].str());
  //     result.posix_mode = std::stoi(matches[6].str());
  //     result.is_open = std::stoi(matches[7].str()) != 0;
  //   }
    
  //   return result;
  // }

  // file_stat(const std::string& str) {
  //   *this = from_string(str);
  // }

  // std::string to_string(file_stat const& arg)
  // {
  //   std::ostringstream ss;
  //   ss << arg;
  //   return std::move(ss).str();  // enable efficiencies in c++17
  // }

  // MSGPACK_DEFINE(fd, file_pointer, file_size, flags, posix_mode, mode, is_open);

  // Serialization
  template <class Archive>
  void
  serialize (Archive &archive)
  {
    auto fh_ = reinterpret_cast<size_t>(this->fh);
    archive (fh_, this->fd, this->file_pointer, this->file_size, this->flags, this->posix_mode, this->mode, this->is_open);
    fh = reinterpret_cast<FILE *>(fh_);
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
  bool special_type; // currently used for fgets, which is an fread that breaks on newline, potentially can be used other unusual call types with special behavior

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
      destination (), meta_updated(false), local_copy(false), check_fs(false), special_type(false)
  {
  }

  // MSGPACK_DEFINE(t_type, iface, task_id, publish, addDataspace, async, source, destination, meta_updated, local_copy, check_fs);

  task (task_type t_type)
    : t_type (t_type), iface (io_client_type::POSIX), task_id (0), publish (true), addDataspace (true),
      async (true), source (), destination (), meta_updated(false), local_copy(false), check_fs(false), special_type(false)
  {
  }
  task (const task &t_other)
    : t_type (t_other.t_type), iface (t_other.iface), task_id (t_other.task_id),
        publish (t_other.publish), addDataspace (t_other.addDataspace),
      async (t_other.async), source (t_other.source), destination(t_other.destination),
      meta_updated(t_other.meta_updated), local_copy(t_other.local_copy), check_fs(t_other.check_fs), special_type(t_other.special_type)
  {
  }

  task(task_type t_type_, const file &source_, const file &destination_) : t_type(t_type_), source(source_), destination(destination_), task_id (0), iface(io_client_type::POSIX), publish (true),
									   addDataspace (true), async (true), meta_updated(false), local_copy(false), check_fs(false), special_type(false)
  {
  }
  task (task_type t_type_, file &source)
    : t_type (t_type_), source (source), task_id (0), publish (true),
      iface(io_client_type::POSIX), addDataspace (true), async (true),
      destination (), meta_updated(false), local_copy(false), check_fs(false),
      special_type(false)
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
    special_type = other.special_type;
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
    archive (t_type_, iface_, this->task_id, this->publish, this->addDataspace, this->async, this->source, this->destination, this->meta_updated, this->local_copy, this->check_fs, this->special_type);
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
