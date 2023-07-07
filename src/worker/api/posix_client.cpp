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
//
// Created by hdevarajan on 5/10/18.
//

#include "posix_client.h"
#include <iomanip>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <hcl/common/debug.h>
#include <dtio/dtio_system.h>

int posix_client::dtio_read(read_task task) {
#ifdef TIMERW
  Timer t = Timer();
  t.resumeTime();
#endif
  int fd = open64(task.source.filename.c_str(), O_RDWR); // "r+"
  auto data = static_cast<char *>(malloc(sizeof(char) * task.source.size));
  long long int pos = lseek64(fd, (off_t)task.source.offset, SEEK_SET);
  if (pos != 0)
    std::cerr << "posix_client::read() seek failed\n";
  // throw std::runtime_error("posix_client::read() seek failed");
  size_t count = read(fd, data, task.source.size); // FIXME rename DTIO API to avoid name conflicts
  if (count != task.source.size)
    std::cerr << "posix_client::read() read failed\n";
  // throw std::runtime_error("posix_client::read() read failed");

  auto map_client = dtio_system::getInstance(WORKER)->map_client();
  serialization_manager sm = serialization_manager();
#ifdef TIMERDM
  Timer t0 = Timer();
  t0.resumeTime();
#endif
  map_client->put(DATASPACE_DB, task.destination.filename, data,
                  std::to_string(task.destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "posix_client::read()::send_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
#endif
  // std::cout<<task.destination.filename<<","<<task.destination.server<<"\n";
  close(fd);
  free(data);
  if (task.local_copy) {
    int file_id = static_cast<int>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count());
    std::string file_path = dir + std::to_string(file_id);
    int fd1 = open64(file_path.c_str(), O_RDWR); // "w+"
    write(fd1, data, task.source.size);
    close(fd1);
    size_t chunk_index = (task.source.offset / MAX_IO_UNIT);
    size_t base_offset =
        chunk_index * MAX_IO_UNIT + task.source.offset % MAX_IO_UNIT;

    chunk_meta chunk_meta1;
    chunk_meta1.actual_user_chunk = task.source;
    chunk_meta1.destination.location = BUFFERS;
    chunk_meta1.destination.filename = file_path;
    chunk_meta1.destination.offset = 0;
    chunk_meta1.destination.size = task.source.size;
    chunk_meta1.destination.worker = worker_index;
#ifdef TIMERMDM
    Timer t1 = Timer();
    t1.resumeTime();
#endif
    std::string chunk_str = sm.serialize_chunk(chunk_meta1);
    map_client->put(table::CHUNK_DB,
                    task.source.filename + std::to_string(base_offset),
                    chunk_str, std::to_string(-1));
#ifdef TIMERMDM
    std::cout << "posix_client::read()::update_meta," << t1.pauseTime() << "\n";
#endif
  }
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "posix_client::read()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  return 0;
}

int posix_client::dtio_write(write_task task) {
#ifdef TIMERW
  Timer t = Timer();
  t.resumeTime();
#endif
  std::shared_ptr<distributed_hashmap> map_client =
      dtio_system::getInstance(WORKER)->map_client();
  std::shared_ptr<distributed_hashmap> map_server =
      dtio_system::getInstance(WORKER)->map_server();
  serialization_manager sm = serialization_manager();
  auto source = task.source;
  size_t chunk_index = (source.offset / MAX_IO_UNIT);
  size_t base_offset = chunk_index * MAX_IO_UNIT + source.offset % MAX_IO_UNIT;
  std::string chunk_str = map_client->get(
      table::CHUNK_DB,
      task.source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
      std::to_string(-1));

  chunk_meta chunk_meta1 = sm.deserialize_chunk(chunk_str);
#ifdef TIMERDM
  Timer t0 = Timer();
  t0.resumeTime();
#endif
  // FIXME: modify the data to pull from local buffers if there is no dataspace (task is sync)
  std::string data = map_client->get(DATASPACE_DB, task.destination.filename,
				     std::to_string(task.destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "posix_client::write()::get_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
#endif
  std::string file_path;
  if (chunk_meta1.destination.location == location_type::CACHE) {
    /*
     * New I/O
     */
    auto file_id = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    file_path = dir + std::to_string(file_id);
#ifdef DEBUG
    std::cout << data.length() << " chunk index:" << chunk_index
              << " dataspaceId:" << task.destination.filename
              << " created filename:" << file_path
              << " clientId: " << task.destination.server << "\n";
#endif
    int fd = open64(file_path.c_str(), O_RDWR); // "w+"
    auto count = write(fd, data.c_str(), task.destination.size);
    if (count != task.destination.size)
      std::cerr << "written less" << count << "\n";
    close(fd);
  } else {
    /*cd
     * existing I/O
     */
#ifdef DEBUG
    std::cout << "update file  " << data.length()
              << " chunk index:" << chunk_index
              << " dataspaceId:" << task.destination.filename
              << " clientId: " << task.destination.server << "\n";
#endif

    file_path = chunk_meta1.destination.filename;
    int fd = open64(chunk_meta1.destination.filename.c_str(), O_RDWR); // "r+"
    lseek64(fd, task.source.offset - base_offset, SEEK_SET);
    write(fd, data.c_str(), task.source.size);
    close(fd);
  }
  chunk_meta1.actual_user_chunk = task.source;
  chunk_meta1.destination.location = BUFFERS;
  chunk_meta1.destination.filename = file_path;
  chunk_meta1.destination.offset = 0;
  chunk_meta1.destination.size = task.destination.size;
  chunk_meta1.destination.worker = worker_index;
#ifdef TIMERMDM
  Timer t1 = Timer();
  t1.resumeTime();
#endif
  chunk_str = sm.serialize_chunk(chunk_meta1);
  map_client->put(table::CHUNK_DB,
                  task.source.filename +
                      std::to_string(chunk_index * MAX_IO_UNIT),
                  chunk_str, std::to_string(-1));
//    map_client->remove(DATASPACE_DB,task.destination.filename,
//    std::to_string(task.destination.server));
#ifdef TIMERMDM
  std::cout << "posix_client::write()::update_meta," << t1.pauseTime() << "\n";
#endif
  map_server->put(table::WRITE_FINISHED_DB, task.destination.filename,
                  std::to_string(-1), std::to_string(-1));
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "posix_client::write()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  return 0;
}

int posix_client::dtio_delete_file(delete_task task) {
  unlink(task.source.filename.c_str());
  // TODO:update metadata of delete
  return 0;
}

int posix_client::dtio_flush_file(flush_task task) {
  int fd_source = open64(task.source.filename.c_str(), O_RDWR); // "rb+"
  int fd_destination = open64(task.destination.filename.c_str(), O_RDWR); // "rb+"
  char *data = static_cast<char *>(malloc(sizeof(char) * task.source.size));
  lseek64(fd_source, task.source.offset, SEEK_SET);
  read(fd_source, data, task.source.size);
  lseek64(fd_destination, task.destination.offset, SEEK_SET);
  write(fd_destination, data, task.destination.size);
  close(fd_destination);
  close(fd_source);
  return 0;
}
