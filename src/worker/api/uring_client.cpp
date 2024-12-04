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

#include "uring_client.h"
#include <iomanip>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <hcl/common/debug.h>
#include <dtio/dtio_system.h>

int uring_client::dtio_read(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
  struct io_uring ring;
  struct io_uring_sqe *sqe;
  struct io_uring_cqe *cqe;
  int ret;
  void *data;

  io_uring_queue_init(URING_QD, &ring, 0);

#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  char *filepath = (strncmp(tsk[task_idx]->source.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->source.filename + 7) : tsk[task_idx]->source.filename;
  int fd;
  fd = open64(filepath, O_RDWR | O_CREAT | O_DIRECT, 0664); // "r+"

  long long int pos = tsk[task_idx]->source.offset; //lseek64(fd, (off_t)tsk[task_idx]->source.offset, SEEK_SET);
  if (pos < 0)
    std::cerr << "uring_client::read() seek failed\n";
  // throw std::runtime_error("uring_client::read() seek failed");

  data = calloc(tsk[task_idx]->source.size, sizeof(char));

  sqe = io_uring_get_sqe(&ring);

  io_uring_prep_read(sqe, fd, data, tsk[task_idx]->source.size, pos);

  io_uring_submit(&ring);

  ret = io_uring_wait_cqe(&ring, &cqe);

  io_uring_cqe_seen(&ring, cqe);

  io_uring_queue_exit(&ring);

  auto map_client = dtio_system::getInstance(WORKER)->map_client();
  auto map_cm_client = dtio_system::getInstance(WORKER)->map_client("metadata+chunkmeta");
#ifdef TIMERDM
  hcl::Timer t0 = hcl::Timer();
  t0.resumeTime();
#endif
  map_client->put(DATASPACE_DB, tsk[task_idx]->source.filename, (char *)data,
		  std::to_string(tsk[task_idx]->destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "uring_client::read()::send_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
#endif
  // std::cout<<tsk[task_idx]->destination.filename<<","<<tsk[task_idx]->destination.server<<"\n";

  close(fd);
  free(data);

  if (tsk[task_idx]->local_copy) {
    int file_id = static_cast<int>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count());
    std::string file_path = dir + std::to_string(file_id);
    int fd1 = open64(file_path.c_str(), O_RDWR | O_CREAT, 0664); // "w+"
    write(fd1, data, tsk[task_idx]->source.size);
    close(fd1);
    size_t chunk_index = (tsk[task_idx]->source.offset / MAX_IO_UNIT);
    size_t base_offset =
        chunk_index * MAX_IO_UNIT + tsk[task_idx]->source.offset % MAX_IO_UNIT;

    chunk_meta chunk_meta1;
    chunk_meta1.actual_user_chunk = tsk[task_idx]->source;
    chunk_meta1.destination.location = BUFFERS;
    strncpy(chunk_meta1.destination.filename, file_path.c_str(), DTIO_FILENAME_MAX);
    chunk_meta1.destination.offset = 0;
    chunk_meta1.destination.size = tsk[task_idx]->source.size;
    chunk_meta1.destination.worker = worker_index;
#ifdef TIMERMDM
    hcl::Timer t1 = hcl::Timer();
    t1.resumeTime();
#endif
    map_cm_client->put(table::CHUNK_DB,
                    tsk[task_idx]->source.filename + std::to_string(base_offset),
		    &chunk_meta1, std::to_string(-1));
#ifdef TIMERMDM
    std::cout << "uring_client::read()::update_meta," << t1.pauseTime() << "\n";
#endif
  }
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "uring_client::read()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  }
  return 0;
}

int uring_client::dtio_write(task *tsk[]) {
  int task_idx;
  struct io_uring ring;
  struct io_uring_sqe *sqe;
  struct io_uring_cqe *cqe;
  int ret;
  std::vector<int> fd;
  char *data_alloc[BATCH_SIZE];

  io_uring_queue_init(URING_QD, &ring, 0);

#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  std::shared_ptr<distributed_hashmap> map_client =
      dtio_system::getInstance(WORKER)->map_client();
  std::shared_ptr<distributed_hashmap> map_server =
      dtio_system::getInstance(WORKER)->map_server();
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
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
    stream << "uring_client::write()::get_data," << std::fixed
	   << std::setprecision(10) << t0.pauseTime() << "\n";
    std::cout << stream.str();
#endif
    // std::string file_path;
    char *filepath = (strncmp(tsk[task_idx]->destination.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->destination.filename + 7) : tsk[task_idx]->destination.filename ;
    fd.emplace_back(open64(filepath, O_RDWR | O_CREAT | O_DIRECT, 0664)); // "w+"

    if (fd.back() < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }

    long long int pos = tsk[task_idx]->destination.offset; //lseek64(fd, tsk[task_idx]->destination.offset, SEEK_SET);
    sqe = io_uring_get_sqe(&ring);

    io_uring_prep_write(sqe, fd.back(), data, tsk[task_idx]->destination.size, pos);

    // if (count != tsk[task_idx]->destination.size)
    //   std::cerr << "written less" << count << "\n";

  }
#else
  {
    data_alloc[task_idx] = (char *)malloc(MAX_IO_UNIT);
    if (!chunk_avail) {
      map_client->get(DATASPACE_DB, tsk[task_idx]->destination.filename,
		      std::to_string(tsk[task_idx]->destination.server), data_alloc[task_idx]);
    }
    else {
      // It's a chunk, this should be the filename with chunk id
      map_client->get(DATASPACE_DB, tsk[task_idx]->source.filename,
		      std::to_string(tsk[task_idx]->destination.server), data_alloc[task_idx]);
    }
#ifdef TIMERDM
    std::stringstream stream;
    stream << "uring_client::write()::get_data," << std::fixed
	   << std::setprecision(10) << t0.pauseTime() << "\n";
    std::cout << stream.str();
#endif
    // std::string file_path;
    char *filepath = (strncmp(tsk[task_idx]->destination.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->destination.filename + 7) : tsk[task_idx]->destination.filename ;
    fd.emplace_back(open64(filepath, O_RDWR | O_CREAT | O_DIRECT, 0664)); // "w+"
    if (fd.back() < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }

    long long int pos = tsk[task_idx]->destination.offset; //lseek64(fd, tsk[task_idx]->destination.offset, SEEK_SET);

    sqe = io_uring_get_sqe(&ring);

    io_uring_prep_write(sqe, fd.back(), data_alloc[task_idx], tsk[task_idx]->destination.size, pos);

    // io_uring_submit(&ring);

    // ret = io_uring_wait_cqe(&ring, &cqe);
    
    // io_uring_cqe_seen(&ring, cqe);

    // io_uring_queue_exit(&ring);

    // free(data);

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
//   std::cout << "uring_client::write()::update_meta," << t1.pauseTime() << "\n";
// #endif
  }
  io_uring_submit(&ring);

  ret = io_uring_wait_cqe(&ring, &cqe);

  io_uring_cqe_seen(&ring, cqe);

  io_uring_queue_exit(&ring);

  while (!fd.empty()) {
    close(fd.back());
    fd.pop_back();
  }

  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
#if(!STACK_ALLOCATION) 
    free(data_alloc[task_idx]);
#endif
    // NOTE Removing this makes I/O asynchronous by default
    map_server->put(table::WRITE_FINISHED_DB, std::to_string(tsk[task_idx]->task_id),
		    std::to_string(-1), std::to_string(-1));
  }

#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "uring_client::write()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  return 0;
}

int uring_client::dtio_delete_file(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
  char *filepath = (strncmp(tsk[task_idx]->source.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->source.filename + 7) : tsk[task_idx]->source.filename ;
  unlink(filepath);
  // TODO:update metadata of delete
  }
  return 0;
}

int uring_client::dtio_flush_file(task *tsk[]) {
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
