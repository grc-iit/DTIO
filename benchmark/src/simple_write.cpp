//
// Created by lukemartinlogan on 7/22/22.
//

#include "util.h"

int main(int argc, char **argv) {
  dtio::MPI_Init(&argc, &argv);
  if (argc != 3) {
    printf("USAGE: ./simple_write [dtio_conf] [filename]\n");
    exit(1);
  }

  FILE *fp;
  int rv; // return val
  char write_buf[50] = "Testing R/W with DTIO. This is msg body.";

  Timer timer = Timer();
  timer.resumeTime();
  std::cerr << "This is a simple WRITE test.\nPerforming OPEN\n";

  // open/create file
  fp = dtio::fopen(argv[2], "w+");
  if (fp == NULL) {
    std::cerr << "Failed to open/create file. Aborting...\n";
    exit(-1);
  }

  // write message to file
  std::cerr << "Performing WRITE\n";
  rv = dtio::fwrite(write_buf, sizeof(write_buf), 1, fp);
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << "Written to: " << argv[2] << "\n";

  std::cerr << "Performing CLOSE\n";
  dtio::fclose(fp);
  timer.pauseTime();
  auto time = timer.getElapsedTime();
  std::cerr << "Time elapsed: " << time << " seconds.\n";

  char read_buf[50];

  timer.resumeTime();
  std::cerr << "This is a simple READ test.\n";

  // open file for reading
  std::cerr << "Performing OPEN\n";
  fp = dtio::fopen(argv[2], "rb");
  if (fp == NULL) {
    std::cerr << "Failed to find file. Aborting...\n";
    exit(-1);
  }

  // read
  std::cerr << "Performing READ\n";
  rv = dtio::fread(read_buf, sizeof(read_buf), 1, fp);
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << read_buf << "\n";

  std::cerr << "Performing CLOSE\n";
  dtio::fclose(fp);
  timer.pauseTime();
  time = timer.getElapsedTime();
  std::cerr << "Time elapsed: " << time << " seconds.\n";

  dtio::MPI_Finalize();
}
