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
  if (argc != 5) {
    printf("USAGE: ./hacc_tabios [dtio_conf] [file_path] [iteration] "
           "[buf_path]\n");
    exit(1);
  }

  std::stringstream stream;
  int rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  if (rank == 0)
    stream << "hacc_base()," << std::fixed << std::setprecision(10);
  std::string file_path = argv[2];
  int iteration = atoi(argv[3]);
  std::string buf_path = argv[4];
  std::string filename = buf_path + "test_" + std::to_string(rank) + ".dat";
  size_t io_per_teration = 32 * 1024 * 1024;
  std::vector<std::array<size_t, 2>> workload =
      std::vector<std::array<size_t, 2>>();
  workload.push_back({1 * 1024 * 1024, 32});
  size_t current_offset = 0;
  Timer global_timer = Timer();
  char *write_buf[32];
  for (int i = 0; i < 32; ++i) {
    write_buf[i] = static_cast<char *>(malloc(1 * 1024 * 1024));
    gen_random(write_buf[i], 1 * 1024 * 1024);
  }
  global_timer.resumeTime();
#ifdef TIMERBASE
  Timer wbb = Timer();
  wbb.resumeTime();
#endif
  //    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  //    int fd=open(filename.c_str(),O_CREAT|O_SYNC|O_RSYNC|O_RDWR|O_TRUNC,
  //    mode);
  FILE *fh = std::fopen(filename.c_str(), "w+");
#ifdef TIMERBASE
  wbb.pauseTime();
#endif
  global_timer.pauseTime();

  for (int i = 0; i < iteration; ++i) {
    for (auto item : workload) {
      for (int j = 0; j < item[1]; ++j) {
        global_timer.resumeTime();
#ifdef TIMERBASE
        wbb.resumeTime();
#endif
        //                write(fd,write_buf,item[0]);
        //                fsync(fd);
        std::fwrite(write_buf[j], sizeof(char), item[0], fh);
        MPI_Barrier(MPI_COMM_WORLD);
        global_timer.pauseTime();
#ifdef TIMERBASE
        wbb.pauseTime();
#endif
        current_offset += item[0];
      }
    }
  }
  for (int i = 0; i < 32; ++i) {
    free(write_buf[i]);
  }
#ifdef TIMERBASE
  wbb.resumeTime();
  if (rank == 0)
    stream << "write_to_BB," << wbb.pauseTime() << ",";
#endif
  auto read_buf = static_cast<char *>(calloc(io_per_teration, sizeof(char)));
  global_timer.resumeTime();

#ifdef TIMERBASE
  Timer rbb = Timer();
  rbb.resumeTime();
#endif
  // close(fd);
  std::fclose(fh);
  //    int in=open(filename.c_str(),O_SYNC|O_RSYNC|O_RDONLY| mode);
  //    read(in,read_buf,io_per_teration);
  //    close(in);
  FILE *fh1 = std::fopen(filename.c_str(), "r");
  std::fread(read_buf, sizeof(char), io_per_teration, fh1);
  std::fclose(fh1);
  MPI_Barrier(MPI_COMM_WORLD);
#ifdef TIMERBASE
  if (rank == 0)
    stream << "read_from_BB," << rbb.pauseTime() << ",";
#endif

  std::string output = file_path + "final_" + std::to_string(rank) + ".out";
#ifdef TIMERBASE
  Timer pfs = Timer();
  pfs.resumeTime();
#endif
  //    int out=open(output.c_str(),O_CREAT|O_SYNC|O_WRONLY|O_TRUNC, mode);
  //    write(out,read_buf,io_per_teration);
  //    fsync(out);
  //    close(out);
  FILE *fh2 = std::fopen(output.c_str(), "w");
  std::fwrite(read_buf, sizeof(char), io_per_teration, fh2);
  std::fflush(fh2);
  std::fclose(fh2);
  MPI_Barrier(MPI_COMM_WORLD);
#ifdef TIMERBASE
  if (rank == 0)
    stream << "write_to_PFS," << pfs.pauseTime() << "\n";
#endif
  global_timer.pauseTime();
  free(read_buf);
  auto time = global_timer.getElapsedTime();
  double sum;
  MPI_Allreduce(&time, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  double mean = sum / comm_size;
  if (rank == 0) {
    stream << "average," << mean << "\n";
    std::cerr << stream.str();
  }

  dtio::MPI_Finalize();
}
