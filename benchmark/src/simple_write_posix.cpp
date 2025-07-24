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

#include <fcntl.h>
#include <mpi.h>
#include <unistd.h>

#include <iostream>

#include "util.h"

int main(int argc, char **argv) {
  int fd;
  int rv;
  std::cout << "Init" << std::endl;
  // dtio::MPI_Init(&argc, &argv);  // Commented out - driver API removed
  MPI_Init(&argc, &argv);  // Use standard MPI instead
  std::cout << "Init done" << std::endl;

  if (argc < 2) {
    printf("USAGE: ./simple_write [filename]\n");
    exit(1);
  }

  static char write_buf[2048];

  for (int i = 0; i < 2048; ++i) {
    write_buf[i] = 'a' + (rand() % 26);
  }

  std::cerr << "This is a simple WRITE test.\n";

  // Write
  std::cout << "Open" << std::endl;
  // fd = dtio::posix::open(argv[1], O_RDWR | O_CREAT | O_TRUNC);  // Commented
  // out - driver API removed
  fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC,
            0644);  // Use standard POSIX instead
  std::cout << "Open done" << std::endl;
  if (fd < 0) {
    std::cerr << "Failed to open/create file. Aborting...\n";
    exit(1);
  }

  std::cout << "Write" << std::endl;
  // rv = dtio::posix::write(fd, write_buf, sizeof(write_buf));  // Commented
  // out - driver API removed
  rv = write(fd, write_buf, sizeof(write_buf));  // Use standard POSIX instead

  if (rv != sizeof(write_buf)) {
    // dtio::posix::close(fd);  // Commented out - driver API removed
    close(fd);  // Use standard POSIX instead
    exit(1);
  }
  std::cout << "Write done" << std::endl;

  // static char read_buf[2048];

  // Read test
  /* DISABLED for now
  // fd = dtio::posix::open(argv[2], O_RDWR);  // Commented out - driver API
  removed fd = open(argv[2], O_RDWR);  // Use standard POSIX instead

  if (fd < 0) {
    std::cerr << "Failed to open file for read. Aborting...\n";
    exit(1);
  }

  // dtio::posix::lseek(fd, 0, SEEK_SET);  // Commented out - driver API removed
  lseek(fd, 0, SEEK_SET);  // Use standard POSIX instead

  // rv = dtio::posix::read(fd, read_buf, sizeof(write_buf));  // Commented out
  - driver API removed rv = read(fd, read_buf, sizeof(write_buf));  // Use
  standard POSIX instead
  */

  // dtio::posix::close(fd);  // Commented out - driver API removed
  close(fd);  // Use standard POSIX instead

  printf("Test passed!\n");

  // dtio::MPI_Finalize();  // Commented out - driver API removed
  MPI_Finalize();  // Use standard MPI instead
  return 0;
}
