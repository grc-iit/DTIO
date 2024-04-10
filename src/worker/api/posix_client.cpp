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

int posix_client::dtio_read(task tsk) {
#ifdef TIMERW
  Timer t = Timer();
  t.resumeTime();
#endif
  int fd = open64(tsk.source.filename, O_RDWR | O_CREAT); // "r+"
  auto data = static_cast<char *>(calloc(tsk.source.size, sizeof(char)));
  long long int pos = lseek64(fd, (off_t)tsk.source.offset, SEEK_SET);
  if (pos != 0)
    std::cerr << "posix_client::read() seek failed\n";
  // throw std::runtime_error("posix_client::read() seek failed");
  size_t count = read(fd, data, tsk.source.size); // FIXME rename DTIO API to avoid name conflicts
  if (count == -1)
    std::cerr << "posix_client::read() read failed\n";
  // throw std::runtime_error("posix_client::read() read failed");

  auto map_client = dtio_system::getInstance(WORKER)->map_client();
#ifdef TIMERDM
  Timer t0 = Timer();
  t0.resumeTime();
#endif
  map_client->put(DATASPACE_DB, tsk.source.filename, data,
		  std::to_string(tsk.destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "posix_client::read()::send_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
#endif
  // std::cout<<tsk.destination.filename<<","<<tsk.destination.server<<"\n";
  close(fd);
  free(data);
  if (tsk.local_copy) {
    int file_id = static_cast<int>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count());
    std::string file_path = dir + std::to_string(file_id);
    int fd1 = open64(file_path.c_str(), O_RDWR | O_CREAT); // "w+"
    write(fd1, data, tsk.source.size);
    close(fd1);
    size_t chunk_index = (tsk.source.offset / MAX_IO_UNIT);
    size_t base_offset =
        chunk_index * MAX_IO_UNIT + tsk.source.offset % MAX_IO_UNIT;

    chunk_meta chunk_meta1;
    chunk_meta1.actual_user_chunk = tsk.source;
    chunk_meta1.destination.location = BUFFERS;
    strncpy(chunk_meta1.destination.filename, file_path.c_str(), DTIO_FILENAME_MAX);
    chunk_meta1.destination.offset = 0;
    chunk_meta1.destination.size = tsk.source.size;
    chunk_meta1.destination.worker = worker_index;
#ifdef TIMERMDM
    Timer t1 = Timer();
    t1.resumeTime();
#endif
    map_client->put(table::CHUNK_DB,
                    tsk.source.filename + std::to_string(base_offset),
		    &chunk_meta1, std::to_string(-1));
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

int posix_client::dtio_write(task tsk) {
#ifdef TIMERW
  Timer t = Timer();
  t.resumeTime();
#endif
  std::shared_ptr<distributed_hashmap> map_client =
      dtio_system::getInstance(WORKER)->map_client();
  std::shared_ptr<distributed_hashmap> map_server =
      dtio_system::getInstance(WORKER)->map_server();
  auto source = tsk.source;
  size_t chunk_index = (source.offset / MAX_IO_UNIT);
  size_t base_offset = chunk_index * MAX_IO_UNIT + source.offset % MAX_IO_UNIT;
  chunk_meta chunk_meta1;
  bool chunk_avail = map_client->get(
      table::CHUNK_DB,
      tsk.source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
      std::to_string(-1), &chunk_meta1);

#ifdef TIMERDM
  Timer t0 = Timer();
  t0.resumeTime();
#endif
  // FIXME: modify the data to pull from local buffers if there is no dataspace (task is sync)
  char data[MAX_IO_UNIT];
  map_client->get(DATASPACE_DB, tsk.destination.filename,
		  std::to_string(tsk.destination.server), data);
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
    std::cout << strlen(data) << " chunk index:" << chunk_index
              << " dataspaceId:" << tsk.destination.filename
              << " created filename:" << file_path
              << " clientId: " << tsk.destination.server << "\n";
#endif
    int fd = open64(file_path.c_str(), O_RDWR | O_CREAT); // "w+"
    auto count = write(fd, data, tsk.destination.size);
    if (count != tsk.destination.size)
      std::cerr << "written less" << count << "\n";
    close(fd);
  } else {
    /*cd
     * existing I/O
     */
#ifdef DEBUG
    std::cout << "update file  " << strlen(data)
              << " chunk index:" << chunk_index
              << " dataspaceId:" << tsk.destination.filename
              << " clientId: " << tsk.destination.server << "\n";
#endif

    file_path = chunk_meta1.destination.filename;
    int fd = open64(chunk_meta1.destination.filename, O_RDWR | O_CREAT); // "r+"
    lseek64(fd, tsk.source.offset - base_offset, SEEK_SET);
    write(fd, data, tsk.source.size);
    close(fd);
  }
  chunk_meta1.actual_user_chunk = tsk.source;
  chunk_meta1.destination.location = BUFFERS;
  strncpy(chunk_meta1.destination.filename, file_path.c_str(), DTIO_FILENAME_MAX);
  chunk_meta1.destination.offset = 0;
  chunk_meta1.destination.size = tsk.destination.size;
  chunk_meta1.destination.worker = worker_index;
#ifdef TIMERMDM
  Timer t1 = Timer();
  t1.resumeTime();
#endif
  map_client->put(table::CHUNK_DB,
                  tsk.source.filename +
		  std::to_string(chunk_index * MAX_IO_UNIT),
                  &chunk_meta1, std::to_string(-1));
//    map_client->remove(DATASPACE_DB,task.destination.filename,
//    std::to_string(task.destination.server));
#ifdef TIMERMDM
  std::cout << "posix_client::write()::update_meta," << t1.pauseTime() << "\n";
#endif
  map_server->put(table::WRITE_FINISHED_DB, tsk.destination.filename,
                  std::to_string(-1), std::to_string(-1));
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "posix_client::write()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  return 0;
}

int posix_client::dtio_delete_file(task tsk) {
  unlink(tsk.source.filename);
  // TODO:update metadata of delete
  return 0;
}

int posix_client::dtio_flush_file(task tsk) {
  int fd_source = open64(tsk.source.filename, O_RDWR); // "rb+"
  int fd_destination = open64(tsk.destination.filename, O_RDWR); // "rb+"
  char *data = static_cast<char *>(malloc(sizeof(char) * tsk.source.size));
  lseek64(fd_source, tsk.source.offset, SEEK_SET);
  read(fd_source, data, tsk.source.size);
  lseek64(fd_destination, tsk.destination.offset, SEEK_SET);
  write(fd_destination, data, tsk.destination.size);
  close(fd_destination);
  close(fd_source);
  return 0;
}
