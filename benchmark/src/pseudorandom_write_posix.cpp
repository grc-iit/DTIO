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
  if (argc != 3 && argc != 4) {
    printf("USAGE: ./simple_write [filename] [num_messages] [max size] [(optional) seed]\n");
    exit(1);
  }

  int fd;
  int rv; // return val
  int i;
  int num_messages = atoi(argv[2]);
  int seed = ((argc == 4) ? atoi(argv[4]) : rand());
  int max_size = atoi(argv[3]);
  char write_buf[max_size];
  std::cerr << "Using random seed " << seed << std::endl;

  for (i = 0; i < max_size; i++) {
    write_buf[i] = (char)(65 + random() % 57); // Just a random char
  }

  std::cerr << "This is a simple WRITE test.\n";

  // open/create file
  fd = dtio::posix::open(argv[1], O_RDWR | O_CREAT | O_TRUNC);
  if (fd < 0) {
    std::cerr << "Failed to open/create file. Aborting...\n";
    exit(-1);
  }

  // write message to file
  srandom(seed);
  for (i = 0; i < num_messages; i++) {

    rv = dtio::posix::write(fd, write_buf, random() % max_size);
  }
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << "Written to: " << argv[1] << "\n";

  dtio::posix::close(fd);

  dtio::MPI_Finalize();
}
