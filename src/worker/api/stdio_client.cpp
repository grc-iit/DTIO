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

#include "stdio_client.h"
#include <iomanip>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <hcl/common/debug.h>
#include <dtio/dtio_system.h>

int stdio_client::dtio_read(task tsk) {
#ifdef TIMERW
  Timer t = Timer();
  t.resumeTime();
#endif
  FILE *fh = fopen(tsk.source.filename, "r+");
  auto data = static_cast<char *>(malloc(sizeof(char) * tsk.source.size));
  long long int pos = fseek(fh, tsk.source.offset, SEEK_SET);
  if (pos != 0)
    std::cerr << "stdio_client::read() seek failed\n";
  // throw std::runtime_error("stdio_client::read() seek failed");
  size_t count = fread(data, sizeof(char), tsk.source.size, fh);
  if (count != tsk.source.size)
    std::cerr << "stdio_client::read() read failed\n";
  // throw std::runtime_error("stdio_client::read() read failed");

  auto map_client = dtio_system::getInstance(WORKER)->map_client();
  serialization_manager sm = serialization_manager();
#ifdef TIMERDM
  Timer t0 = Timer();
  t0.resumeTime();
#endif
  map_client->put(DATASPACE_DB, tsk.destination.filename, data,
                  std::to_string(tsk.destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "stdio_client::read()::send_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
#endif
  // std::cout<<task.destination.filename<<","<<task.destination.server<<"\n";
  fclose(fh);
  free(data);
  if (tsk.local_copy) {
    int file_id = static_cast<int>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count());
    std::string file_path = dir + std::to_string(file_id);
    FILE *fh1 = fopen(file_path.c_str(), "w+");
    fwrite(data, tsk.source.size, sizeof(char), fh1);
    fclose(fh1);
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
    std::string chunk_str = sm.serialize_chunk(chunk_meta1);
    map_client->put(table::CHUNK_DB,
                    tsk.source.filename + std::to_string(base_offset),
                    chunk_str, std::to_string(-1));
#ifdef TIMERMDM
    std::cout << "stdio_client::read()::update_meta," << t1.pauseTime() << "\n";
#endif
  }
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "stdio_client::read()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  return 0;
}

int stdio_client::dtio_write(task tsk) {
#ifdef TIMERW
  Timer t = Timer();
  t.resumeTime();
#endif
  std::shared_ptr<distributed_hashmap> map_client =
      dtio_system::getInstance(WORKER)->map_client();
  std::shared_ptr<distributed_hashmap> map_server =
      dtio_system::getInstance(WORKER)->map_server();
  serialization_manager sm = serialization_manager();
  auto source = tsk.source;
  size_t chunk_index = (source.offset / MAX_IO_UNIT);
  size_t base_offset = chunk_index * MAX_IO_UNIT + source.offset % MAX_IO_UNIT;
  std::string chunk_str = map_client->get(
      table::CHUNK_DB,
      tsk.source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
      std::to_string(-1));

  chunk_meta chunk_meta1 = sm.deserialize_chunk(chunk_str);
#ifdef TIMERDM
  Timer t0 = Timer();
  t0.resumeTime();
#endif
  // FIXME: modify the data to pull from local buffers if there is no dataspace (task is sync)
  std::string data = map_client->get(DATASPACE_DB, tsk.destination.filename,
				     std::to_string(tsk.destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "stdio_client::write()::get_data," << std::fixed
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
              << " dataspaceId:" << tsk.destination.filename
              << " created filename:" << file_path
              << " clientId: " << tsk.destination.server << "\n";
#endif
    FILE *fh = fopen(file_path.c_str(), "w+");
    auto count = fwrite(data.c_str(), sizeof(char), tsk.destination.size, fh);
    if (count != tsk.destination.size)
      std::cerr << "written less" << count << "\n";
    fclose(fh);
  } else {
    /*cd
     * existing I/O
     */
#ifdef DEBUG
    std::cout << "update file  " << data.length()
              << " chunk index:" << chunk_index
              << " dataspaceId:" << tsk.destination.filename
              << " clientId: " << tsk.destination.server << "\n";
#endif

    file_path = chunk_meta1.destination.filename;
    FILE *fh = fopen(chunk_meta1.destination.filename, "r+");
    fseek(fh, tsk.source.offset - base_offset, SEEK_SET);
    fwrite(data.c_str(), sizeof(char), tsk.source.size, fh);
    fclose(fh);
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
  chunk_str = sm.serialize_chunk(chunk_meta1);
  map_client->put(table::CHUNK_DB,
                  tsk.source.filename +
                      std::to_string(chunk_index * MAX_IO_UNIT),
                  chunk_str, std::to_string(-1));
//    map_client->remove(DATASPACE_DB,task.destination.filename,
//    std::to_string(task.destination.server));
#ifdef TIMERMDM
  std::cout << "stdio_client::write()::update_meta," << t1.pauseTime() << "\n";
#endif
  map_server->put(table::WRITE_FINISHED_DB, tsk.destination.filename,
                  std::to_string(-1), std::to_string(-1));
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "stdio_client::write()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  return 0;
}

int stdio_client::dtio_delete_file(task tsk) {
  remove(tsk.source.filename);
  // TODO:update metadata of delete
  return 0;
}

int stdio_client::dtio_flush_file(task tsk) {
  FILE *fh_source = fopen(tsk.source.filename, "rb+");
  FILE *fh_destination = fopen(tsk.destination.filename, "rb+");
  char *data = static_cast<char *>(malloc(sizeof(char) * tsk.source.size));
  fseek(fh_source, tsk.source.offset, SEEK_SET);
  fread(data, tsk.source.size, sizeof(char), fh_source);
  fseek(fh_destination, tsk.destination.offset, SEEK_SET);
  fwrite(data, tsk.destination.size, sizeof(char), fh_destination);
  fclose(fh_destination);
  fclose(fh_source);
  return 0;
}
