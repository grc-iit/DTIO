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
  if (argc != 4) {
    printf("USAGE: ./cm1_tabios [dtio_conf] [file_path] [iteration]\n");
    exit(1);
  }

#ifdef COLLECT
  system("sh /home/cc/nfs/aetrio/scripts/log_reset.sh");
#endif

  std::stringstream stream;
  int rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  stream << "cm1_tabios," << std::fixed << std::setprecision(10);
  std::string file_path = argv[2];
  int iteration = atoi(argv[3]);
  std::string filename = file_path + "/test_" + std::to_string(rank) + ".dat";
  size_t io_per_teration = 32 * 1024 * 1024;

  printf("HERE0\n");

  std::vector<std::array<size_t, 2>> workload =
      std::vector<std::array<size_t, 2>>();
  workload.push_back({1 * 1024 * 1024, 32});

  size_t current_offset = 0;
  Timer global_timer = Timer();

  global_timer.resumeTime();
  FILE *fh = dtio::fopen(filename.c_str(), "w+");
  global_timer.pauseTime();

  char *write_buf[32];
  for (int i = 0; i < 32; ++i) {
    write_buf[i] = static_cast<char *>(malloc(1 * 1024 * 1024));
    gen_random(write_buf[i], 1 * 1024 * 1024);
  }
  std::vector<std::pair<size_t, std::vector<task *>>> operations =
      std::vector<std::pair<size_t, std::vector<task *>>>();
  printf("HERE1\n");
  for (int i = 0; i < iteration; ++i) {
    for (auto item : workload) {
      for (int j = 0; j < item[1]; ++j) {
        global_timer.resumeTime();
        operations.emplace_back(std::make_pair(
            item[0],
            dtio::fwrite_async(write_buf[j], sizeof(char), item[0], fh)));
        global_timer.pauseTime();
        current_offset += item[0];
      }
    }
  }
  printf("HERE2\n");
  global_timer.resumeTime();
  for (auto operation : operations) {
    auto bytes = dtio::fwrite_wait(operation.second);
    printf("HERE2.5: %ul\n", bytes);
    if (bytes != operation.first) std::cerr << "Write failed\n";
  }
  global_timer.pauseTime();
  for (int i = 0; i < 32; ++i) {
    free(write_buf[i]);
  }
  global_timer.resumeTime();
  if (rank == 0) std::cerr << "Write finished\n";
  dtio::fclose(fh);
  global_timer.pauseTime();
  printf("HERE3");

  auto time = global_timer.getElapsedTime();
  double sum, max, min;
  MPI_Allreduce(&time, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  MPI_Allreduce(&time, &max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&time, &min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
  double mean = sum / comm_size;
  if (rank == 0) {
#ifdef COLLECT
    double ts = get_average_ts();
    double worker = get_average_worker();
    stream << ts << "," << worker << ",";
#endif
    stream << mean << "," << max << "," << min << "\n";
    std::cerr << stream.str();
  }
  dtio::MPI_Finalize();
}
