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

  int fd;
  int rv; // return val
  char write_buf[50] = "Testing R/W with DTIO. This is msg body.";

  Timer timer = Timer();
  timer.resumeTime();
  std::cerr << "This is a simple WRITE test.\n";

  // open/create file
  fd = dtio::posix::open(argv[2], O_RDWR | O_CREAT | O_TRUNC);
  if (fd < 0) {
    std::cerr << "Failed to open/create file. Aborting...\n";
    exit(-1);
  }

  // write message to file
  rv = dtio::posix::write(fd, write_buf, sizeof(write_buf));
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << "Written to: " << argv[2] << "\n";

  dtio::posix::close(fd);
  timer.pauseTime();
  auto time = timer.getElapsedTime();
  std::cerr << "Time elapsed: " << time << " seconds.\n";

  char read_buf[50];

  timer.resumeTime();
  std::cerr << "This is a simple READ test.\n";

  // open file for reading
  fd = dtio::posix::open(argv[2], O_RDWR);
  if (fd < 0) {
    std::cerr << "Failed to find file. Aborting...\n";
    exit(-1);
  }

  // read
  rv = dtio::posix::read(fd, read_buf, sizeof(read_buf));
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << read_buf << "\n";

  dtio::posix::close(fd);
  timer.pauseTime();
  time = timer.getElapsedTime();
  std::cerr << "Time elapsed: " << time << " seconds.\n";

  dtio::MPI_Finalize();
}
