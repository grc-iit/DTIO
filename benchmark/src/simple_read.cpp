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
  if (argc != 3) {
    printf("USAGE: ./simple_read [dtio_conf] [filename]\n");
    exit(1);
  }

  FILE *fp;
  int rv;  // return val
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
