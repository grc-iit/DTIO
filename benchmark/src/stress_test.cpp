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

#include "util.h"

int main(int argc, char **argv) {
  dtio::MPI_Init(&argc, &argv);
  if (argc != 2) {
    printf("USAGE: ./stress_test [dtio_conf]\n");
    exit(1);
  }

  int rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  if (rank == 0) std::cerr << "Stress test\n";
  const int io_size = 1024 * 1024;
  int num_iterations = 128;
  Timer global_timer = Timer();
  global_timer.resumeTime();
  FILE *fh = dtio::fopen("file.test", "w+");
  global_timer.pauseTime();
  std::vector<std::pair<size_t, std::vector<task *>>> operations =
      std::vector<std::pair<size_t, std::vector<task *>>>();
  char write_buf[io_size];
  gen_random(write_buf, io_size);
  for (int i = 0; i < num_iterations; ++i) {
    global_timer.resumeTime();
    operations.emplace_back(std::make_pair(
        io_size, dtio::fwrite_async(write_buf, sizeof(char), io_size, fh)));
    global_timer.pauseTime();
    if (i != 0 && i % 32 == 0) {
      if (rank == 0) std::cerr << "Waiting..." << i << "\n";
      for (int task = 0; task < 32; ++task) {
        auto operation = operations[task];
        global_timer.resumeTime();
        auto bytes = dtio::fwrite_wait(operation.second);
        if (bytes != operation.first) std::cerr << "Write failed\n";
        global_timer.pauseTime();
      }
      operations.erase(operations.begin(), operations.begin() + 32);
      MPI_Barrier(MPI_COMM_WORLD);
    }
  }
  global_timer.resumeTime();
  for (auto operation : operations) {
    auto bytes = dtio::fwrite_wait(operation.second);
    if (bytes != operation.first) std::cerr << "Write failed\n";
  }
  global_timer.pauseTime();
  global_timer.resumeTime();
  dtio::fclose(fh);
  global_timer.pauseTime();
  if (rank == 0) std::cerr << "Done writing. Now reducing\n";
  auto time = global_timer.getElapsedTime();
  double sum, max, min;
  MPI_Allreduce(&time, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  MPI_Allreduce(&time, &max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&time, &min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
  double mean = sum / comm_size;
  if (rank == 0) {
    std::stringstream stream;
    std::cerr << "Write finished\n";
    stream << "mean," << mean << "\tmax," << max << "\tmin," << min << "\n";
    std::cerr << stream.str();
  }
  dtio::MPI_Finalize();
}
