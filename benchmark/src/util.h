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

#ifndef DTIO_BENCH_UTIL_H
#define DTIO_BENCH_UTIL_H

#include <fcntl.h>

#include "dtio/drivers/posix.h"
#include "dtio/drivers/stdio.h"
#include "dtio/return_codes.h"
#include "dtio/utilities.h"
/* #include <fstream> */
#include <malloc.h>
#include <mpi.h>

#include <iomanip>
#include <random>
/* #include <sstream> */
#include <zconf.h>

/* static void gen_random(char *s, std::size_t len) { */
/*   static const char alphanum[] = "0123456789" */
/*                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" */
/*                                  "abcdefghijklmnopqrstuvwxyz"; */
/*   std::default_random_engine generator; */
/*   std::uniform_int_distribution<int> dist(1, 1000000); */
/*   for (int i = 0; i < len; ++i) { */
/*     s[i] = alphanum[dist(generator) % (sizeof(alphanum) - 1)]; */
/*   } */

/*   s[len] = 0; */
/* } */

/* static float get_average_ts() { */
/*   /\* run system command du -s to calculate size of director. *\/ */
/*   std::string cmd = "sh /home/cc/nfs/aetrio/scripts/calc_ts.sh"; */
/*   FILE *fp; */
/*   std::array<char, 128> buffer; */
/*   std::string result; */
/*   std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose); */
/*   if (!pipe) */
/*     throw std::runtime_error("popen() failed!"); */
/*   while (!feof(pipe.get())) { */
/*     if (fgets(buffer.data(), 128, pipe.get()) != nullptr) */
/*       result += buffer.data(); */
/*   } */
/*   return std::stof(result); */
/* } */

/* static float get_average_worker() { */
/*   /\* run system command du -s to calculate size of director. *\/ */
/*   std::string cmd = "sh /home/cc/nfs/aetrio/scripts/calc_worker.sh"; */
/*   FILE *fp; */
/*   std::array<char, 128> buffer; */
/*   std::string result; */
/*   std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose); */
/*   if (!pipe) */
/*     throw std::runtime_error("popen() failed!"); */
/*   while (!feof(pipe.get())) { */
/*     if (fgets(buffer.data(), 128, pipe.get()) != nullptr) */
/*       result += buffer.data(); */
/*   } */
/*   return std::stof(result); */
/* } */

/* static void wait_for_read(size_t size, std::vector<task> tasks, */
/*                           std::string filename) { */
/*   char read_buf[size]; */
/*   auto bytes = dtio::fread_wait(read_buf, tasks, filename); */
/*   if (bytes != size) */
/*     std::cerr << "Read failed:" << bytes << "\n"; */
/* } */

#endif  // DTIO_BENCH_UTIL_H
