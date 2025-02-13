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

#include "hdf5_client.h"
#include "H5Tpublic.h"
#include "dtio/common/constants.h"
#include <iomanip>
#include <dtio/common/client_interface/distributed_hashmap.h>
#include <hcl/common/debug.h>
#include <dtio/dtio_system.h>
#include <vector>

int hdf5_client::dtio_stage(task *tsk[], char *staging_space) {
}

int hdf5_client::dtio_read(task *tsk[], char *staging_space) {
  int task_idx;
  for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
#ifdef TIMERW
  hcl::Timer t = hcl::Timer();
  t.resumeTime();
#endif
  auto map_client = dtio_system::getInstance(WORKER)->map_client();
  auto map_fm_client = dtio_system::getInstance(WORKER)->map_client("metadata+filemeta");

  // Query the metadata maps to get the datatasks associated with a file
  file_meta fm;
  // std::cout << "Filemeta retrieval" << std::endl;
  map_fm_client->get(table::FILE_CHUNK_DB, tsk[task_idx]->source.filename, std::to_string(-1), &fm);

  std::vector<file> resolve_dts;

  // Query those datatasks for range
  // std::cout << "Query dts for range" << std::endl;
  int *range_bound = (int *)malloc(tsk[task_idx]->source.size * sizeof(int));
  range_bound[0] = 0; //tsk[task_idx]->source.offset;
  // std::cout << "Populate range bound" << std::endl;
  for (int i = 1; i < tsk[task_idx]->source.size; ++i) {
    range_bound[i] = range_bound[i-1] + 1;
  }
  // std::cout << "Range requests" << std::endl;
  int range_lower = 0; //tsk[task_idx]->source.offset;
  int range_upper = tsk[task_idx]->source.size; // tsk[task_idx]->source.offset + 
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

  char *path = (strncmp(tsk[task_idx]->source.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->source.filename + 7) : tsk[task_idx]->source.filename ;
  char *dsetname = strstr(path, "h5") + 2; // One for the / delimeter
  char *filepath = strndupa(path, (size_t)(dsetname - path)); // Allocates on stack, but it's just a filepath so should be ok. If it isn't, use strndup instead
  hid_t file, dset_id;

  std::cout << "Opening file " << filepath << std::endl;
  if (temp_fd == -1) {
    file = H5Fopen(filepath, H5F_ACC_RDWR, H5P_DEFAULT); // "w+"
    temp_fd = file;
  }
  else {
    file = temp_fd;
  }
  if (file < 0) {
    std::cerr << "File " << filepath << " didn't open" << std::endl;
  }

  std::cout << "Opening dataset " << dsetname << std::endl;
  dset_id = H5Dopen(file, dsetname, H5P_DEFAULT);

  std::cout << "Dataspace stuff" << std::endl;
  hid_t dataspace_id = H5Dget_space(dset_id);
  int ndims = H5Sget_simple_extent_ndims(dataspace_id);
  hsize_t dims[ndims];
  H5Sget_simple_extent_dims(dataspace_id, dims, NULL);

  int count = 2; // For testing

  int i;
  for (i = 0; i < ndims; i++) {
    count *= dims[i];
  }

   // Use this and the source.size to determine the selection

  hsize_t hslab_offset[ndims];
  hsize_t hslab_count[ndims];
  hsize_t mem_dims[1];
  int offset = tsk[task_idx]->source.offset;
  int size = tsk[task_idx]->source.size;

  std::cout << "Calculating chunk size" << std::endl;

  int chunk_size = 1;
  mem_dims[0] = 1;
  for (i = 1; i < ndims; i++) {
    hslab_offset[i] = 0;
    hslab_count[i] = dims[i];
    mem_dims[0] *= dims[i];
    chunk_size *= dims[i];
  }

  // std::cout << "Calculating hyperslab offset" << std::endl;
  hslab_offset[0] = 0;
  // hslab_offset[0] = offset / (chunk_size * 2);
  while (offset > 0) {
    hslab_offset[0]++;
    if (hslab_offset[0] > dims[0]) {
      std::cerr << "HDF5 worker client giving offset beyond end of the dataset" << std::endl;
    }
    offset -= chunk_size * 2;
  }

  // std::cout << "Calculating hyperslab count" << std::endl;
  hslab_count[0] = std::max(1, size / (chunk_size * 2));
  int datasize = std::pow(chunk_size * 2, hslab_count[0]);
  // hslab_count[0] = 0;
  // int datasize = 1;
  // while (size > 0) {
  //   hslab_count[0]++;
  //   datasize *= chunk_size * 2;
  //   if (hslab_count[0] > dims[0]) {
  //     std::cerr << "HDF5 worker client giving count beyond end of the dataset" << std::endl;
  //   }
  //   size -= chunk_size * 2;
  // }
  mem_dims[0] *= hslab_count[0];

  // std::cout << "Allocating data" << std::endl;
  void *data = calloc(datasize, sizeof(char *));

  // Resolve range on current task
  std::cout << "Pulling from resolved range" << std::endl;
  if (range_resolved) {
    for (unsigned i = resolve_dts.size(); i-- > 0; ) {
      // Currently, we're just iterating backwards so newer DTs overwrite older ones.
      // TODO better way to do this is to resolve the range by precalculating the offsets and sizes that get pulled into the buffer from each DT.
      map_client->get(DATASPACE_DB, resolve_dts[i].filename, std::to_string(resolve_dts[i].server), static_cast<char *>(data) + resolve_dts[i].offset - 0,
		      tsk[task_idx]->source.size - resolve_dts[i].offset + 0); // 0 should be tsk[task_idx]->source.offset
      // Make sure we get only the size number of elements, and start from the correct offset that is achieved by the DT.
    }
    map_client->put(DATASPACE_DB, tsk[task_idx]->source.filename, static_cast<char *>(data), datasize,
		  std::to_string(tsk[task_idx]->destination.server));
    // std::cout << static_cast<char *>(data) << std::endl;
    free(data);
    continue;
  }

  // std::cout << "Selecting hyperslab" << std::endl;
  H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, hslab_offset, NULL, hslab_count, NULL);

  // std::cout << "Performing read offsets " << hslab_offset[0] << " " << hslab_offset[1] << " " << hslab_offset[2] << " " << hslab_offset[3] << std:: endl;
  // std::cout << "Performing read counts " << hslab_count[0] << " " << hslab_count[1] << " " << hslab_count[2] << " " << hslab_count[3] << std:: endl;
  // std::cout << "Performing read datasize " << datasize << std:: endl;
  // std::cout << "Performing read memdims " << mem_dims[0] << std:: endl;
  
  hid_t memspace_id = H5Screate_simple(1, mem_dims, NULL);

  H5Dread(dset_id, H5T_STD_U16LE, memspace_id, dataspace_id, H5P_DEFAULT, data);

  // std::cout << "Read done" << std::endl;

#ifdef TIMERDM
  hcl::Timer t0 = hcl::Timer();
  t0.resumeTime();
#endif


  map_client->put(DATASPACE_DB, tsk[task_idx]->source.filename, static_cast<char *>(data), datasize,
		  std::to_string(tsk[task_idx]->destination.server));
#ifdef TIMERDM
  std::stringstream stream;
  stream << "hdf5_client::read()::send_data," << std::fixed
         << std::setprecision(10) << t0.pauseTime() << "\n";
  std::cout << stream.str();
#endif
  // std::cout<<tsk[task_idx]->destination.filename<<","<<tsk[task_idx]->destination.server<<"\n";
  // H5Fclose(file);
  free(data);
  }
  return 0;
}

int hdf5_client::dtio_write(task *tsk[]) {
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
  // chunk_meta chunk_meta1;
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
    stream << "hdf5_client::write()::get_data," << std::fixed
	   << std::setprecision(10) << t0.pauseTime() << "\n";
    std::cout << stream.str();
#endif
    // std::string file_path;
    char *path = (strncmp(tsk[task_idx]->destination.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->destination.filename + 7) : tsk[task_idx]->destination.filename ;
    char *dsetname = strstr(path, "h5") + 2;
    char *filepath = strndupa(path, (size_t)(dsetname - path)); // Allocates on stack, but it's just a filepath so should be ok. If it isn't, use strndup instead
    hid_t file, dset_id;
    // H5Fcreate(filepath, H5F_ACC_RDWR, H5P_DEFAULT, H5P_DEFAULT);
    file = H5Fopen(filepath, H5F_ACC_RDWR, H5P_DEFAULT); // "w+"
    if (file < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }

    // H5Dcreate(file, dsetname, H5T_STD_U16LE, NULL, H5P_DEFAULT, NULL, H5P_DEFAULT);
    dset_id = H5Dopen(file, dsetname, H5P_DEFAULT);

    H5Dwrite(dset_id, H5T_STD_U16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    //lseek64(fd, tsk[task_idx]->destination.offset, SEEK_SET);
    // auto count = write(fd, data, tsk[task_idx]->destination.size);
    // if (count != tsk[task_idx]->destination.size)
    //   std::cerr << "written less" << count << "\n";
    H5Fclose(file);
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
    stream << "hdf5_client::write()::get_data," << std::fixed
	   << std::setprecision(10) << t0.pauseTime() << "\n";
    std::cout << stream.str();
#endif
    // std::string file_path;
    char *path = (strncmp(tsk[task_idx]->destination.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->destination.filename + 7) : tsk[task_idx]->destination.filename ;
    char *dsetname = strstr(path, "h5") + 2;
    char *filepath = strndupa(path, (size_t)(dsetname - path)); // Allocates on stack, but
    hid_t file, dset_id;
    file = H5Fopen(filepath, H5F_ACC_RDWR, H5P_DEFAULT); // "w+"
    if (file < 0) {
      std::cerr << "File " << filepath << " didn't open" << std::endl;
    }

    dset_id = H5Dopen(file, dsetname, H5P_DEFAULT);

    std::cout << "Doing H5Dwrite" << std::endl;
    H5Dwrite(dset_id, H5T_STD_U16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    std::cout << "H5Dwrite done" << std::endl;
    //lseek64(fd, tsk[task_idx]->destination.offset, SEEK_SET);
    // auto count = write(fd, data, tsk[task_idx]->destination.size);
    // if (count != tsk[task_idx]->destination.size)
    //   std::cerr << "written less" << count << "\n";
    H5Fclose(file);
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
//   std::cout << "hdf5_client::write()::update_meta," << t1.pauseTime() << "\n";
// #endif
  // NOTE Removing this makes I/O asynchronous by default
  map_server->put(table::WRITE_FINISHED_DB, std::to_string(tsk[task_idx]->task_id),
                  std::to_string(-1), std::to_string(-1));
#ifdef TIMERW
  std::stringstream stream1;
  stream1 << "hdf5_client::write()," << std::fixed << std::setprecision(10)
          << t.pauseTime() << "\n";
  std::cout << stream1.str();
#endif
  }
  return 0;
}

int hdf5_client::dtio_delete_file(task *tsk[]) {
  // int task_idx;
  // for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
  // char *filepath = (strncmp(tsk[task_idx]->source.filename, "dtio://", 7) == 0) ? (tsk[task_idx]->source.filename + 7) : tsk[task_idx]->source.filename ;
  // unlink(filepath);
  // // TODO:update metadata of delete
  // }
  // return 0;
}

int hdf5_client::dtio_flush_file(task *tsk[]) {
  // int task_idx;
  // for (task_idx = 0; task_idx < BATCH_SIZE; task_idx++) {
  // int fd_source = open64(tsk[task_idx]->source.filename, O_RDWR); // "rb+"
  // int fd_destination = open64(tsk[task_idx]->destination.filename, O_RDWR); // "rb+"
  // char *data = static_cast<char *>(malloc(sizeof(char) * tsk[task_idx]->source.size));
  // lseek64(fd_source, tsk[task_idx]->source.offset, SEEK_SET);
  // read(fd_source, data, tsk[task_idx]->source.size);
  // lseek64(fd_destination, tsk[task_idx]->destination.offset, SEEK_SET);
  // write(fd_destination, data, tsk[task_idx]->destination.size);
  // close(fd_destination);
  // close(fd_source);
  // }
  // return 0;
}
