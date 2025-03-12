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

#include "stdio_client.h"
#include <iomanip>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <hcl/common/debug.h>
#include <dtio/dtio_system.h>

int stdio_client::dtio_stage(task *tsk[], char *staging_space) {
  auto map_client = dtio_system::getInstance(WORKER)->map_client();
  // auto map_cm_client = dtio_system::getInstance(WORKER)->map_client("metadata+chunkmeta");
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  char *filepath = (strncmp(tsk[task_idx]->source.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->source.filename + 7) : tsk[task_idx]->source.filename;

  FILE *fh = fopen(filepath, "rw+"); // "r+"
  temp_fh = fh;
  size_t count;
  while ((count = fread(staging_space, sizeof(char), tsk[task_idx]->source.size, fh)) != 0);

  map_client->put(table::STAGING_DB, tsk[task_idx]->source.filename, std::to_string(worker_index), std::to_string(-1));

  }
  return 0;
}

int stdio_client::dtio_read(task *tsk[], char *staging_space) {
  auto map_client = dtio_system::getInstance(WORKER)->map_client();
  auto map_fm_client = dtio_system::getInstance(WORKER)->map_client("metadata+filemeta");
  auto mdm = metadata_manager::getInstance(WORKER);
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {

#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  char *filepath = (strncmp(tsk[task_idx]->source.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->source.filename + 7) : tsk[task_idx]->source.filename;

  if (tsk[task_idx]->source.offset + tsk[task_idx]->source.size < ConfigManager::get_instance()->WORKER_STAGING_SIZE && tsk[task_idx]->source.offset >= 0) {
    mdm->update_read_task_info (tsk, tsk[0]->source.filename);
    map_client->put(DATASPACE_DB, tsk[task_idx]->source.filename,
		    &staging_space[tsk[task_idx]->source.offset],
		    tsk[task_idx]->source.size,
		    std::to_string(tsk[task_idx]->destination.server));
    return 0;
  }
  FILE *fh;
  if (strcmp(temp_fn.c_str(), tsk[task_idx]->source.filename) != 0) {
    fh = fopen(filepath, "rw+");
    temp_fh = fh;
    temp_fn = tsk[task_idx]->source.filename;
  }
  else {
    fh = temp_fh;
  }
  auto data = static_cast<char *>(calloc(tsk[task_idx]->source.size, sizeof(char)));
  int pos = fseek(fh, tsk[task_idx]->source.offset, SEEK_SET);
  if (pos != 0)
    std::cerr << "stdio_client::read() seek failed\n";
  // throw std::runtime_error("stdio_client::read() seek failed");
  size_t count;
  if (tsk[task_idx]->special_type) {
    char *result;
    result = fgets(data, tsk[task_idx]->source.size, fh);
    if (result == NULL) {
      count = -1;
    }
    else {
      char *comp;
      comp = strchr(data, '\n');
      if (comp != NULL) {
	count = (size_t)(comp - data) + 1;
      }
      else {
	count = tsk[task_idx]->source.size;
      }
    }
  }
  else {
    count = fread(data, sizeof(char), tsk[task_idx]->source.size, fh);
  }
  if (count == -1)
    std::cerr << "stdio_client::read() read failed\n";
  // throw std::runtime_error("stdio_client::read() read failed");

#ifdef TIMERDM
  hcl::Timer t0 = hcl::Timer();
  t0.resumeTime();
#endif
  // std::cout << "data " << data << std::endl;
  tsk[task_idx]->source.size = count; // Quick fix for metadata, now metadata will update correctly for corner cases like fgets
  // if (count > 1024*1024*1024*5 || tsk[task_idx]->source.size > 1024*1024*1024*5) {
  //   std::cout << "Weird task spotted " << tsk[task_idx]->special_type << " " << count << " " << tsk[task_idx]->source.size << std::endl;
  // }
  mdm->update_read_task_info (tsk, tsk[0]->source.filename);
  map_client->put(DATASPACE_DB, tsk[task_idx]->source.filename, data,
		  tsk[task_idx]->source.size,
                  std::to_string(tsk[task_idx]->destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "stdio_client::read()::send_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
#endif
  // std::cout<<task.destination.filename<<","<<task.destination.server<<"\n";
  // fclose(fh);
  free(data);
  // if (tsk[task_idx]->local_copy) {
//     int file_id = static_cast<int>(
//         duration_cast<milliseconds>(system_clock::now().time_since_epoch())
//             .count());
//     std::string file_path = dir + std::to_string(file_id);
//     FILE *fh1 = fopen(file_path.c_str(), "w+");
//     fwrite(data, tsk[task_idx]->source.size, sizeof(char), fh1);
//     fclose(fh1);
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
//     map_cm_client->put(table::CHUNK_DB,
//                     tsk[task_idx]->source.filename + std::to_string(base_offset),
//                     &chunk_meta1, std::to_string(-1));
// #ifdef TIMERMDM
//     std::cout << "stdio_client::read()::update_meta," << t1.pauseTime() << "\n";
// #endif
//   }
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "stdio_client::read()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  }
  return 0;
}

int stdio_client::dtio_write(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  auto mdm = metadata_manager::getInstance(WORKER);

  std::shared_ptr<distributed_hashmap> map_client =
      dtio_system::getInstance(WORKER)->map_client();
  std::shared_ptr<distributed_hashmap> map_server =
      dtio_system::getInstance(WORKER)->map_server();
  auto source = tsk[task_idx]->source;
  size_t chunk_index = (source.offset / MAX_IO_UNIT);
  size_t base_offset = chunk_index * MAX_IO_UNIT + source.offset % MAX_IO_UNIT;
  // std::string chunk_str = map_cm_client->get(
  //     table::CHUNK_DB,
  //     tsk[task_idx]->source.filename + std::to_string(chunk_index * MAX_IO_UNIT),
  //     std::to_string(-1));

  // chunk_meta chunk_meta1;
  bool chunk_avail = !tsk[task_idx]->addDataspace;
#ifdef TIMERDM
  hcl::Timer t0 = hcl::Timer();
  t0.resumeTime();
#endif
  // FIXME: modify the data to pull from local buffers if there is no dataspace (task is sync)
#if(STACK_ALLOCATION)
  {
    char *data;
    data = (char *)malloc(MAX_IO_UNIT);
    // char data[MAX_IO_UNIT];
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
    FILE *fp;
    if (strcmp(temp_fn.c_str(), tsk[task_idx]->source.filename) != 0) {
      fp = fopen(filepath, "rw+");
      temp_fh = fp;
      temp_fn = tsk[task_idx]->source.filename;
    }
    else {
      fp = temp_fh;
    }
    if (fp == nullptr) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }
    fseek(fp, tsk[task_idx]->destination.offset, SEEK_SET);
    auto count = fwrite(data, sizeof(char), tsk[task_idx]->destination.size, fp);
    if (count != tsk[task_idx]->destination.size)
      std::cerr << "written less" << count << "\n";
    free(data);
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
    FILE *fp;
    if (strcmp(temp_fn.c_str(), tsk[task_idx]->source.filename) != 0) {
      fp = fopen(filepath, "rw+");
      temp_fh = fp;
      temp_fn = tsk[task_idx]->source.filename;
    }
    else {
      fp = temp_fh;
    }
    if (fp == nullptr) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }
    fseek(fp, tsk[task_idx]->destination.offset, SEEK_SET);
    auto count = fwrite(data, sizeof(char), tsk[task_idx]->destination.size, fp);
    if (count != tsk[task_idx]->destination.size)
      std::cerr << "written less" << count << "\n";
    free(data);
    // fclose(fp);
  }
#endif
#ifdef TIMERDM
  std::stringstream stream;
  stream << "stdio_client::write()::get_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
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
//     std::cout << data.length() << " chunk index:" << chunk_index
//               << " dataspaceId:" << tsk[task_idx]->destination.filename
//               << " created filename:" << file_path
//               << " clientId: " << tsk[task_idx]->destination.server << "\n";
// #endif
//     FILE *fh = fopen(file_path.c_str(), "w+");
//     auto count = fwrite(data.c_str(), sizeof(char), tsk[task_idx]->destination.size, fh);
//     if (count != tsk[task_idx]->destination.size)
//       std::cerr << "written less" << count << "\n";
//     fclose(fh);
//   } else {
//     /*cd
//      * existing I/O
//      */
// #ifdef DEBUG
//     std::cout << "update file  " << data.length()
//               << " chunk index:" << chunk_index
//               << " dataspaceId:" << tsk[task_idx]->destination.filename
//               << " clientId: " << tsk[task_idx]->destination.server << "\n";
// #endif

//     file_path = chunk_meta1.destination.filename;
//     FILE *fh = fopen(chunk_meta1.destination.filename, "r+");
//     fseek(fh, tsk[task_idx]->source.offset - base_offset, SEEK_SET);
//     fwrite(data.c_str(), sizeof(char), tsk[task_idx]->source.size, fh);
//     fclose(fh);
//   }
//   chunk_meta1.actual_user_chunk = tsk[task_idx]->source;
//   chunk_meta1.destination.location = BUFFERS;
//   strncpy(chunk_meta1.destination.filename, file_path.c_str(), DTIO_FILENAME_MAX);
//   chunk_meta1.destination.offset = 0;
//   chunk_meta1.destination.size = tsk[task_idx]->destination.size;
//   chunk_meta1.destination.worker = worker_index;
// #ifdef TIMERMDM
//   hcl::Timer t1 = hcl::Timer();
//   t1.resumeTime();
// #endif
//   chunk_str = sm.serialize_chunk(chunk_meta1);
//   map_client->put(table::CHUNK_DB,
//                   tsk[task_idx]->source.filename +
//                       std::to_string(chunk_index * MAX_IO_UNIT),
//                   chunk_str, std::to_string(-1));
// //    map_client->remove(DATASPACE_DB,task.destination.filename,
// //    std::to_string(task.destination.server));
// #ifdef TIMERMDM
//   std::cout << "stdio_client::write()::update_meta," << t1.pauseTime() << "\n";
// #endif
  mdm->update_write_task_info (tsk, tsk[0]->destination.filename);
  if (!ConfigManager::get_instance()->ASYNC) {
    map_server->put(table::WRITE_FINISHED_DB, std::to_string(tsk[task_idx]->task_id),
		    std::to_string(-1), std::to_string(-1));
  }
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "stdio_client::write()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  }
  return 0;
}

int stdio_client::dtio_delete_file(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
  remove(tsk[task_idx]->source.filename);
  // TODO:update metadata of delete
  }
  return 0;
}

int stdio_client::dtio_flush_file(task *tsk[]) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
  FILE *fh_source = fopen(tsk[task_idx]->source.filename, "rb+");
  FILE *fh_destination = fopen(tsk[task_idx]->destination.filename, "rb+");
  char *data = static_cast<char *>(malloc(sizeof(char) * tsk[task_idx]->source.size));
  fseek(fh_source, tsk[task_idx]->source.offset, SEEK_SET);
  fread(data, tsk[task_idx]->source.size, sizeof(char), fh_source);
  fseek(fh_destination, tsk[task_idx]->destination.offset, SEEK_SET);
  fwrite(data, tsk[task_idx]->destination.size, sizeof(char), fh_destination);
  fclose(fh_destination);
  fclose(fh_source);
  }
  return 0;
}
