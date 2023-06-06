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
#include "dtio/common/enumerations.h"
#include <dtio/common/utilities.h>
#include <dtio/drivers/mpi.h>

int dtio::MPI_Init(int *argc, char ***argv) {
  PMPI_Init(argc, argv);
  ConfigManager::get_instance()->LoadConfig((*argv)[1]);
  int rank;
  PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
  PMPI_Comm_split(MPI_COMM_WORLD, CLIENT_COLOR, rank, ConfigManager::get_instance()->PROCESS_COMM);
  dtio_system::getInstance(service::LIB);
  return 0;
}

void dtio::MPI_Finalize() { PMPI_Finalize(); }
