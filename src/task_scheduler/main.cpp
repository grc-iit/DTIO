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
/*******************************************************************************
 * Created by hariharan on 5/9/18.
 * Updated by akougkas on 6/26/2018
 ******************************************************************************/
/******************************************************************************
 *include files
 ******************************************************************************/
#include "task_scheduler.h"
#include <dtio/common/data_structures.h>
#include <dtio/common/utilities.h>
#include <mpi.h>
#include <dtio/common/logger.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
/******************************************************************************
 *Main
 ******************************************************************************/
int
main (int argc, char **argv)
{
  MPI_Init (&argc, &argv);
  ConfigManager::get_instance ()->LoadConfig (argv[1]);
  int rank;
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  // Assumption: one worker manager
  MPI_Comm_split (MPI_COMM_WORLD, SCHEDULER_COLOR,
                  rank - ConfigManager::get_instance ()->NUM_WORKERS - 1,
                  &ConfigManager::get_instance ()->PROCESS_COMM);
  int process_rank;
  MPI_Comm_rank(ConfigManager::get_instance ()->PROCESS_COMM, &process_rank);

  MPI_Comm_split (MPI_COMM_WORLD, DATASPACE_COLOR, rank - 1,
                  &ConfigManager::get_instance ()->DATASPACE_COMM);
  MPI_Comm_split (MPI_COMM_WORLD, METADATA_COLOR, rank - 1,
                  &ConfigManager::get_instance ()->METADATA_COMM);
  for (int i = 0; i < ConfigManager::get_instance ()->NUM_WORKERS; i++) {
    MPI_Comm_split (MPI_COMM_WORLD, QUEUE_WORKER_COLOR, process_rank + 1,
		    &ConfigManager::get_instance ()->QUEUE_WORKER_COMM[i]);
  }

  auto scheduler_service = task_scheduler::getInstance (TASK_SCHEDULER);
  DTIO_LOG_DEBUG("[Task Scheduler] Created");
  scheduler_service->run ();
  MPI_Finalize ();
  return 0;
}
