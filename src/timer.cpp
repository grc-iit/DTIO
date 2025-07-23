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

#include <dtio/timer.h>

#include <iomanip>
#include <iostream>

void dtio::Timer::startTime() {
  t1 = std::chrono::high_resolution_clock::now();
}

double dtio::Timer::endTimeWithPrint(std::string fnName) {
  auto t2 = std::chrono::high_resolution_clock::now();
  auto t =
      std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() /
      1000000000.0;
  if (t > 0.001) {
    printf("%s : %lf\n", fnName.c_str(), t);
  }
  return t;
}

double dtio::Timer::stopTime() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now() - t1)
             .count() /
         1000000000.0;
}

double dtio::Timer::pauseTime() {
  elapsed_time += std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::high_resolution_clock::now() - t1)
                      .count() /
                  1000000000.0;
  return elapsed_time;
}

int dtio::Timer::resumeTime() {
  t1 = std::chrono::high_resolution_clock::now();
  return 0;
}
