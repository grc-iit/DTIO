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
 * WORKER MAIN
 * Created by hariharan on 5/10/18.
 * Updated by akougkas on 6/26/2018
 ******************************************************************************/
/******************************************************************************
 *include files
 ******************************************************************************/
#include "dtio/common/enumerations.h"
#include "worker.h"
#include <iostream>
#include <dtio/common/utilities.h>
#include <mpi.h>
/******************************************************************************
 *Worker main
 ******************************************************************************/
int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  ConfigManager::get_instance()->LoadConfig(argv[1]);
  int worker_index;
  MPI_Comm_rank(MPI_COMM_WORLD, &worker_index);
  // Rank in the dataspace communicator should be 0-max_worker_id followed by max_worker_id + 1 - max_client_id
  MPI_Comm_split(MPI_COMM_WORLD, WORKER_COLOR, worker_index+1, ConfigManager::get_instance()->PROCESS_COMM);
  std::shared_ptr<worker> worker_service_i =
      worker::getInstance(service::WORKER, worker_index);
  worker_service_i->run();
  MPI_Finalize();
  return 0;
}
