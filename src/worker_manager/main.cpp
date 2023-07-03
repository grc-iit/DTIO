/*
 * Copyright (C) 2023  SCS Lab <scs-help@cs.iit.edu>, Hariharan
 * Devarajan <hdevarajan@hawk.iit.edu>, Anthony Kougkas
 * <akougkas@iit.edu>, Xian-He Sun <sun@iit.edu>
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
#include "worker_manager_service.h"
#include <dtio/common/utilities.h>
#include <iostream>
#include <mpi.h>

int
main (int argc, char **argv)
{
  MPI_Init (&argc, &argv);
  ConfigManager::get_instance ()->LoadConfig (argv[1]);
  int rank;
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_split (MPI_COMM_WORLD, WORKER_COLOR, 0,
                  &ConfigManager::get_instance ()->PROCESS_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, DATASPACE_NULL_COLOR, 0,
                  &ConfigManager::get_instance ()->DATASPACE_COMM);

  MPI_Comm_split (MPI_COMM_WORLD, QUEUE_CLIENT_COLOR,
                  rank - ConfigManager::get_instance ()->NUM_WORKERS
                      - ConfigManager::get_instance ()->NUM_SCHEDULERS - 1,
                  &ConfigManager::get_instance ()->QUEUE_CLIENT_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, QUEUE_WORKER_COLOR, rank - 1,
                  &ConfigManager::get_instance ()->QUEUE_WORKER_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, QUEUE_TASKSCHED_COLOR, rank - 1,
                  &ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM);
  std::shared_ptr<worker_manager_service> worker_manager_service_i
      = worker_manager_service::getInstance (service::WORKER_MANAGER);
  worker_manager_service_i->run ();
  MPI_Finalize ();
  return 0;
}
