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
#include "worker_manager_service.h"
#include <dtio/common/logger.h>
#include <dtio/common/utilities.h>
#include <iostream>
#include <mpi.h>

int
main (int argc, char **argv)
{
  int provided;
  MPI_Init_thread (&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided < MPI_THREAD_MULTIPLE) {
    printf("Threading wrong\n");
    exit(EXIT_FAILURE);
  }
  // MPI_Init (&argc, &argv);
  ConfigManager::get_instance ()->LoadConfig (argv[1]);
  int rank;
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_split (MPI_COMM_WORLD, WORKER_COLOR, 0,
                  &ConfigManager::get_instance ()->PROCESS_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, DATASPACE_NULL_COLOR, 0,
                  &ConfigManager::get_instance ()->DATASPACE_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, METADATA_NULL_COLOR, 0,
                  &ConfigManager::get_instance ()->METADATA_COMM);

  for (int i = 0; i < ConfigManager::get_instance ()->NUM_WORKERS; i++)
    {
      MPI_Comm_split (MPI_COMM_WORLD, QUEUE_WORKER_NULL_COLOR, 0,
                      &ConfigManager::get_instance ()->QUEUE_WORKER_COMM[i]);
    }
  MPI_Comm_split (MPI_COMM_WORLD, QUEUE_TS_NULL_COLOR, 0,
		  &ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM);
  DTIO_LOG_INFO ("[Worker Manager] Creating");
  std::shared_ptr<worker_manager_service> worker_manager_service_i
      = worker_manager_service::getInstance (service::WORKER_MANAGER);
  DTIO_LOG_INFO ("[Worker Manager] Created");
  worker_manager_service_i->run ();
  MPI_Finalize ();
  return 0;
}
