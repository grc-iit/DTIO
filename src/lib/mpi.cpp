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
//
// Created by anthony on 4/24/18.
//
#include "mpi.h"
#include "dtio/common/enumerations.h"
#include <dtio/common/utilities.h>
#include <dtio/drivers/mpi.h>

int
dtio::MPI_Init (int *argc, char ***argv)
{
  int provided;
  PMPI_Init_thread (argc, argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided < MPI_THREAD_MULTIPLE)
    {
      std::cerr << "Didn't receive appropriate thread specification"
                << std::endl;
    }
  ConfigManager::get_instance ()->LoadConfig ((*argv)[1]);
  int rank;
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_split (MPI_COMM_WORLD, CLIENT_COLOR,
                  rank - ConfigManager::get_instance ()->NUM_WORKERS
                      - ConfigManager::get_instance ()->NUM_SCHEDULERS - 1,
                  &ConfigManager::get_instance ()->PROCESS_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, DATASPACE_COLOR, rank - 1,
                  &ConfigManager::get_instance ()->DATASPACE_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, QUEUE_CLIENT_COLOR, rank - 1,
                  &ConfigManager::get_instance ()->QUEUE_CLIENT_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, QUEUE_WORKER_COLOR, rank - 1,
                  &ConfigManager::get_instance ()->QUEUE_WORKER_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, QUEUE_TASKSCHED_COLOR, rank - 1,
                  &ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM);
  dtio_system::getInstance (service::LIB);
  return 0;
}

void
dtio::MPI_Finalize ()
{
  PMPI_Finalize ();
}
