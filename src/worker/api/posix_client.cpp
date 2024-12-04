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

#include "posix_client.h"
#include "dtio/common/constants.h"
#include <iomanip>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <hcl/common/debug.h>
#include <dtio/dtio_system.h>
#include <vector>

int posix_client::dtio_read(task *tsk[]) {
  auto map_client = dtio_system::getInstance(WORKER)->map_client();
  // auto map_cm_client = dtio_system::getInstance(WORKER)->map_client("metadata+chunkmeta");
  auto map_fm_client = dtio_system::getInstance(WORKER)->map_client("metadata+filemeta");
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  char *filepath = (strncmp(tsk[task_idx]->source.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->source.filename + 7) : tsk[task_idx]->source.filename;

  // Query the metadata maps to get the datatasks associated with a file
  file_meta fm;
  // std::cout << "Filemeta retrieval" << std::endl;
  map_fm_client->get(table::FILE_CHUNK_DB, tsk[task_idx]->source.filename, std::to_string(-1), &fm);

  std::vector<file> resolve_dts;

  // Query those datatasks for range
  // std::cout << "Query dts for range" << std::endl;
  int *range_bound = (int *)malloc(tsk[task_idx]->source.size * sizeof(int));
  range_bound[0] = tsk[task_idx]->source.offset;
  // std::cout << "Populate range bound" << std::endl;
  for (int i = 1; i < tsk[task_idx]->source.size; ++i) {
    range_bound[i] = range_bound[i-1] + 1;
  }
  // std::cout << "Range requests" << std::endl;
  int range_lower = tsk[task_idx]->source.offset;
  int range_upper = tsk[task_idx]->source.offset + tsk[task_idx]->source.size;
  bool range_resolved = false;
  file *curr_chunk;
  for (int i = 0; i < fm.num_chunks; ++i) {
    // std::cout << "i is " << i << std::endl;
    // std::cout << "Check 1" << std::endl;
    if (fm.current_chunk_index - i - 1 >= 0) {
      // std::cout << "Condition A " << fm.current_chunk_index << std::endl;
      curr_chunk = &(fm.chunks[fm.current_chunk_index - i - 1].actual_user_chunk);
      
      // std::cout << "Check 2" << std::endl;
      if (dtio_system::getInstance(WORKER)->range_resolve(&range_bound, tsk[task_idx]->source.size, range_lower, range_upper, curr_chunk->offset, curr_chunk->offset + curr_chunk->size, &range_resolved)) {
	// std::cout << "Push" << std::endl;
	resolve_dts.push_back(*curr_chunk);
      }
      if (range_resolved) {
	break;
      }
    }
    else {
      // std::cout << "Condition B" << std::endl;
      curr_chunk = &(fm.chunks[CHUNK_LIMIT + fm.current_chunk_index - i - 1].actual_user_chunk);
      // std::cout << "Check 2" << std::endl;
      if (dtio_system::getInstance(WORKER)->range_resolve(&range_bound, tsk[task_idx]->source.size, range_lower, range_upper, curr_chunk->offset, curr_chunk->offset + curr_chunk->size, &range_resolved)) {
	// std::cout << "Push" << std::endl;
	resolve_dts.push_back(*curr_chunk);
      }
      if (range_resolved) {
	break;
      }
    }
  }

  // Check if the read can be performed from buffers. To avoid fragmentation, we read from disk when any part of the read buffer isn't in DTIO.
  // TODO it would be far better to allow some fragmentation, but this requires significant changes to the read code and considerations about split and sieved read
  // std::cout << "Check if read can be performed from buffers" << std::endl;
  if (!range_resolved) {
    range_resolved = true;
    // std::cout << "Source size " << tsk[task_idx]->source.size << std::endl;
    // std::cout << "Destination size " << tsk[task_idx]->destination.size << std::endl;
    for (int i = 0; i < tsk[task_idx]->source.size; i++) {
      if (range_bound[i] != -1) {
	std::cout << "Range not resolved at " << range_bound[i] << std::endl;
	range_resolved = false;
	break;
      }
    }
  }
  free(range_bound);
  // Range resolved on current tasks lower down

  int fd;
  if (temp_fd == -1) {
    fd = open64(filepath, O_RDWR | O_CREAT, 0664); // "r+"
    temp_fd = fd;
  }
  else {
    fd = temp_fd;
  }
  chunk_meta chunk_meta1;
  // map_client->get(
  //     table::CHUNK_DB,
  //     tsk[task_idx]->source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
  //     std::to_string(-1), &chunk_meta1);

  // FIXME work on this, chunks should be created using DataTask IDs, and there should be an operation to query DataTask IDs for an anticipated system state
  size_t chunk_index = (tsk[task_idx]->source.offset / MAX_IO_UNIT);
  // map_cm_client->get(table::CHUNK_DB,
  // 		     tsk[task_idx]->source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
  // 		     std::to_string(-1), &chunk_meta1);

  auto data = static_cast<char *>(calloc(tsk[task_idx]->source.size, sizeof(char)));

  // Resolve range on current task
  if (range_resolved) {
    for (unsigned i = resolve_dts.size(); i-- > 0; ) {
      // Currently, we're just iterating backwards so newer DTs overwrite older ones.
      // TODO better way to do this is to resolve the range by precalculating the offsets and sizes that get pulled into the buffer from each DT.
      map_client->get(DATASPACE_DB, resolve_dts[i].filename, std::to_string(resolve_dts[i].server), data + resolve_dts[i].offset - tsk[task_idx]->source.offset,
		      tsk[task_idx]->source.size - resolve_dts[i].offset + tsk[task_idx]->source.offset);
      // Make sure we get only the size number of elements, and start from the correct offset that is achieved by the DT.
    }
    map_client->put(DATASPACE_DB, tsk[task_idx]->source.filename, data,
		    std::to_string(tsk[task_idx]->destination.server));
    // std::cout << data << std::endl;
    free(data);
    continue;
  }


  long long int pos = lseek64(fd, (off_t)tsk[task_idx]->source.offset, SEEK_SET);
  if (pos < 0)
    std::cerr << "posix_client::read() seek failed\n";
  // throw std::runtime_error("posix_client::read() seek failed");
  size_t count = read(fd, data, tsk[task_idx]->source.size);
  if (count == -1)
    std::cerr << "posix_client::read() read failed\n";
  // throw std::runtime_error("posix_client::read() read failed");

#ifdef TIMERDM
  hcl::Timer t0 = hcl::Timer();
  t0.resumeTime();
#endif
  map_client->put(DATASPACE_DB, tsk[task_idx]->source.filename, data,
		  std::to_string(tsk[task_idx]->destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "posix_client::read()::send_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
#endif
  // std::cout<<tsk[task_idx]->destination.filename<<","<<tsk[task_idx]->destination.server<<"\n";
  // close(fd);
  free(data);
//   if (tsk[task_idx]->local_copy) {
//     int file_id = static_cast<int>(
//         duration_cast<milliseconds>(system_clock::now().time_since_epoch())
//             .count());
//     std::string file_path = dir + std::to_string(file_id);
//     int fd1 = open64(file_path.c_str(), O_RDWR | O_CREAT, 0664); // "w+"
//     write(fd1, data, tsk[task_idx]->source.size);
//     close(fd1);
//     size_t chunk_index = (tsk[task_idx]->source.offset / MAX_IO_UNIT);
//     size_t base_offset =
//         chunk_index * MAX_IO_UNIT + tsk[task_idx]->source.offset % MAX_IO_UNIT;

//     chunk_meta chunk_meta1;
//     chunk_meta1.actual_user_chunk = tsk[task_idx]->source;
//     chunk_meta1.destination.location = BUFFERS;
//     strncpy(chunk_meta1.destination.filename, file_path.c_str(), DTIO_FILENAME_MAX);
//     chunk_meta1.destination.offset = 0;
//     chunk_meta1.destination.size = tsk[task_idx]->source.size;
//     chunk_meta1.destination.worker = worker_index;
// #ifdef TIMERMDM
//     hcl::Timer t1 = hcl::Timer();
//     t1.resumeTime();
// #endif
//     // map_cm_client->put(table::CHUNK_DB,
//     //                 tsk[task_idx]->source.filename + std::to_string(base_offset),
//     // 		    &chunk_meta1, std::to_string(-1));
// #ifdef TIMERMDM
//     std::cout << "posix_client::read()::update_meta," << t1.pauseTime() << "\n";
// #endif
//   }
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "posix_client::read()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  }
  return 0;
}

int posix_client::dtio_write(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  std::shared_ptr<distributed_hashmap> map_client =
      dtio_system::getInstance(WORKER)->map_client();
  std::shared_ptr<distributed_hashmap> map_server =
      dtio_system::getInstance(WORKER)->map_server();
  auto source = tsk[task_idx]->source;
  size_t chunk_index = (source.offset / MAX_IO_UNIT);
  size_t base_offset = chunk_index * MAX_IO_UNIT + source.offset % MAX_IO_UNIT;
  chunk_meta chunk_meta1;
  bool chunk_avail = !tsk[task_idx]->addDataspace;
  // map_client->get(
  //     table::CHUNK_DB,
  //     tsk[task_idx]->source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
  //     std::to_string(-1), &chunk_meta1);

#ifdef TIMERDM
  hcl::Timer t0 = hcl::Timer();
  t0.resumeTime();
#endif
  // FIXME: modify the data to pull from local buffers if there is no dataspace (task is sync)
#if(STACK_ALLOCATION)
  {
    // char *data;
    // data = (char *)malloc(MAX_IO_UNIT);
    char data[MAX_IO_UNIT];
    if (!chunk_avail) {
      map_client->get(DATASPACE_DB, tsk[task_idx]->destination.filename,
		      std::to_string(tsk[task_idx]->destination.server), data);
    }
    else {
      // It's a chunk, this should be the filename with chunk id
      map_client->get(DATASPACE_DB, tsk[task_idx]->source.filename,
		      std::to_string(tsk[task_idx]->destination.server), data);
    }
#ifdef TIMERDM
    std::stringstream stream;
    stream << "posix_client::write()::get_data," << std::fixed
	   << std::setprecision(10) << t0.pauseTime() << "\n";
    std::cout << stream.str();
#endif
    // std::string file_path;
    char *filepath = (strncmp(tsk[task_idx]->destination.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->destination.filename + 7) : tsk[task_idx]->destination.filename ;
    int fd;
    if (temp_fd == -1) {
      fd = open64(filepath, O_RDWR | O_CREAT, 0664); // "w+"
      temp_fd = fd;
    }
    else {
      fd = temp_fd;
    }
    if (fd < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }
    lseek64(fd, tsk[task_idx]->destination.offset, SEEK_SET);
    auto count = write(fd, data, tsk[task_idx]->destination.size);
    if (count != tsk[task_idx]->destination.size)
      std::cerr << "written less" << count << "\n";
    // free(data);
    // close(fd);
  }
#else
  {
    char *data = (char *)malloc(MAX_IO_UNIT);
    if (!chunk_avail) {
      map_client->get(DATASPACE_DB, tsk[task_idx]->destination.filename,
		      std::to_string(tsk[task_idx]->destination.server), data);
    }
    else {
      // It's a chunk, this should be the filename with chunk id
      map_client->get(DATASPACE_DB, tsk[task_idx]->source.filename,
		      std::to_string(tsk[task_idx]->destination.server), data);
    }
#ifdef TIMERDM
    std::stringstream stream;
    stream << "posix_client::write()::get_data," << std::fixed
	   << std::setprecision(10) << t0.pauseTime() << "\n";
    std::cout << stream.str();
#endif
    // std::string file_path;
    char *filepath = (strncmp(tsk[task_idx]->destination.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->destination.filename + 7) : tsk[task_idx]->destination.filename ;
    int fd;
    if (temp_fd == -1) {
      fd = open64(filepath, O_RDWR | O_CREAT, 0664); // "w+"
      temp_fd = fd;
    }
    else {
      fd = temp_fd;
    }
    if (fd < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }
    lseek64(fd, tsk[task_idx]->destination.offset, SEEK_SET);
    auto count = write(fd, data, tsk[task_idx]->destination.size);
    if (count != tsk[task_idx]->destination.size)
      std::cerr << "written less" << count << "\n";
    free(data);
    // close(fd);
  }
#endif
//   if (chunk_meta1.destination.location == location_type::CACHE) {
//     /*
//      * New I/O
//      */
//     auto file_id = static_cast<int64_t>(
//         std::chrono::duration_cast<std::chrono::microseconds>(
//             std::chrono::system_clock::now().time_since_epoch())
//             .count());
//     file_path = dir + std::to_string(file_id);
// #ifdef DEBUG
//     std::cout << strlen(data) << " chunk index:" << chunk_index
//               << " dataspaceId:" << tsk[task_idx]->destination.filename
//               << " created filename:" << file_path
//               << " clientId: " << tsk[task_idx]->destination.server << "\n";
// #endif
//     int fd = open64(file_path.c_str(), O_RDWR | O_CREAT); // "w+"
//     auto count = write(fd, data, tsk[task_idx]->destination.size);
//     if (count != tsk[task_idx]->destination.size)
//       std::cerr << "written less" << count << "\n";
//     close(fd);
//   } else {
//     /*cd
//      * existing I/O
//      */
// #ifdef DEBUG
//     std::cout << "update file  " << strlen(data)
//               << " chunk index:" << chunk_index
//               << " dataspaceId:" << tsk[task_idx]->destination.filename
//               << " clientId: " << tsk[task_idx]->destination.server << "\n";
// #endif

//     file_path = chunk_meta1.destination.filename;
//     int fd = open64(chunk_meta1.destination.filename, O_RDWR | O_CREAT); // "r+"
//     lseek64(fd, tsk[task_idx]->source.offset - base_offset, SEEK_SET);
//     write(fd, data, tsk[task_idx]->source.size);
//     close(fd);
//   }
// chunk_meta1.actual_user_chunk = tsk[task_idx]->source;
// chunk_meta1.destination.location = BUFFERS;
// strncpy(chunk_meta1.destination.filename, file_path.c_str(), DTIO_FILENAME_MAX);
// chunk_meta1.destination.offset = 0;
// chunk_meta1.destination.size = tsk[task_idx]->destination.size;
// chunk_meta1.destination.worker = worker_index;
// #ifdef TIMERMDM
//   hcl::Timer t1 = hcl::Timer();
//   t1.resumeTime();
// #endif
// map_client->put(table::CHUNK_DB,
//                 tsk[task_idx]->source.filename +
// 		  std::to_string(chunk_index * MAX_IO_UNIT),
//                 &chunk_meta1, std::to_string(-1));

//    map_client->remove(DATASPACE_DB,task.destination.filename,
//    std::to_string(task.destination.server));
// #ifdef TIMERMDM
//   std::cout << "posix_client::write()::update_meta," << t1.pauseTime() << "\n";
// #endif
  // NOTE Removing this makes I/O asynchronous by default
  map_server->put(table::WRITE_FINISHED_DB, std::to_string(tsk[task_idx]->task_id),
                  std::to_string(-1), std::to_string(-1));
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "posix_client::write()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  }
  return 0;
}

int posix_client::dtio_delete_file(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
  char *filepath = (strncmp(tsk[task_idx]->source.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->source.filename + 7) : tsk[task_idx]->source.filename ;
  unlink(filepath);
  // TODO:update metadata of delete
  }
  return 0;
}

int posix_client::dtio_flush_file(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
  int fd_source = open64(tsk[task_idx]->source.filename, O_RDWR); // "rb+"
  int fd_destination = open64(tsk[task_idx]->destination.filename, O_RDWR); // "rb+"
  char *data = static_cast<char *>(malloc(sizeof(char) * tsk[task_idx]->source.size));
  lseek64(fd_source, tsk[task_idx]->source.offset, SEEK_SET);
  read(fd_source, data, tsk[task_idx]->source.size);
  lseek64(fd_destination, tsk[task_idx]->destination.offset, SEEK_SET);
  write(fd_destination, data, tsk[task_idx]->destination.size);
  close(fd_destination);
  close(fd_source);
  }
  return 0;
}
