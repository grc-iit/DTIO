//
// Created by lukemartinlogan on 7/22/22.
//

#include "util.h"

int main(int argc, char **argv) {
  dtio::MPI_Init(&argc, &argv);
  if (argc != 2) {
    printf("USAGE: ./simple_write [dtio_conf]\n");
    exit(1);
  }

  FILE *fp;
  int rv; // return val
  char write_buf[50] = "Testing R/W with DTIO. This is msg body.";

  Timer timer = Timer();
  timer.resumeTime();
  std::cerr << "This is a simple WRITE test.\n";

  // open/create file
  fp = dtio::fopen(argv[2], "w+");
  if (fp == NULL) {
    std::cerr << "Failed to open/create file. Aborting...\n";
    exit(-1);
  }

  // write message to file
  rv = dtio::fwrite(write_buf, sizeof(write_buf), 1, fp);
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << "Written to: " << argv[2] << "\n";

  dtio::fclose(fp);
  timer.pauseTime();
  auto time = timer.elapsed_time;
  std::cerr << "Time elapsed: " << time << " seconds.\n";
  dtio::MPI_Finalize();
}
