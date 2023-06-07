//
// Created by lukemartinlogan on 7/22/22.
//

#include "util.h"

int main(int argc, char **argv) {
  dtio::MPI_Init(&argc, &argv);
  if (argc != 2) {
    printf("USAGE: ./simple_read [dtio_conf]\n");
    exit(1);
  }

  FILE *fp;
  int rv; // return val
  char read_buf[50];

  Timer timer = Timer();
  timer.resumeTime();
  std::cerr << "This is a simple READ test.\n";

  // open file for reading
  fp = dtio::fopen(argv[2], "rb");
  if (fp == NULL) {
    std::cerr << "Failed to find file. Aborting...\n";
    exit(-1);
  }

  // read
  rv = dtio::fread(read_buf, sizeof(read_buf), 1, fp);
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << read_buf << "\n";

  dtio::fclose(fp);
  timer.pauseTime();
  auto time = timer.getElapsedTime();
  std::cerr << "Time elapsed: " << time << " seconds.\n";
  dtio::MPI_Finalize();
}
