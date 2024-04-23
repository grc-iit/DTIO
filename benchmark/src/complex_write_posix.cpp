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
  MPI_Init(&argc, &argv);
  if (argc != 6) {
    printf("USAGE: ./simple_write [filename] [size1] [size2] [writes_per_phase] [num_phases]\n");
    exit(1);
  }

  int fd;
  int rv; // return val
  int i, j;
  
  int write_size1, write_size2;
  int writes_per_phase, num_phases;

  write_size1 = atoi(argv[2]);
  write_size2 = atoi(argv[3]);
  writes_per_phase = atoi(argv[4]);
  num_phases = atoi(argv[5]);

  int bigger_write = write_size1 > write_size2 ? write_size1 : write_size2;
  char write_buf[bigger_write];
  for (i = 0; i < bigger_write; i++) {
    write_buf[i] = (char)(65 + random() % 57); // Just a random char
  }
  Timer timer = Timer();
  timer.resumeTime();
  std::cerr << "This is a simple WRITE test.\n";

  // open/create file
  fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC);
  if (fd < 0) {
    std::cerr << "Failed to open/create file. Aborting...\n";
    exit(-1);
  }

  // write message to file
  for (i = 0; i < num_phases; i++) {
    for (j = 0; j < writes_per_phase; j++) {
      rv = write(fd, write_buf, write_size1);
    }
    for (j = 0; j < writes_per_phase; j++) {
      rv = write(fd, write_buf, write_size2);
    }
  }
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << "Written to: " << argv[1] << "\n";

  close(fd);
  timer.pauseTime();
  auto time = timer.getElapsedTime();
  std::cerr << "Time elapsed: " << time << " seconds.\n";

  char read_buf[50];

  timer.resumeTime();
  std::cerr << "This is a simple READ test.\n";

  // open file for reading
  fd = (argv[1], O_RDWR);
  if (fd < 0) {
    std::cerr << "Failed to find file. Aborting...\n";
    exit(-1);
  }

  // read
  rv = read(fd, read_buf, sizeof(read_buf));
  std::cerr << "(Return value: " << rv << ")\n";
  std::cerr << read_buf << "\n";

  close(fd);
  timer.pauseTime();
  time = timer.getElapsedTime();
  std::cerr << "Time elapsed: " << time << " seconds.\n";

  MPI_Finalize();
}
