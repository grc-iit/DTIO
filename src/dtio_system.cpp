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
/******************************************************************************
 *include files
 ******************************************************************************/
#include <dtio/common/solver/default_solver.h>
#include <dtio/common/solver/dp_solver.h>
#include <dtio/common/solver/random_solver.h>
#include <dtio/common/solver/round_robin_solver.h>
#include <dtio/dtio_system.h>

std::shared_ptr<dtio_system> dtio_system::instance = nullptr;
/******************************************************************************
 *Interface
 ******************************************************************************/
void dtio_system::init(service service) {
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_size);
  comm_size = comm_size == 0 ? 1 : comm_size;
  if (map_impl_type_t == map_impl_type::MEMCACHE_D) {
    map_server_ = std::make_shared<MemcacheDImpl>(
        service, ConfigManager::get_instance()->MEMCACHED_URL_SERVER, 0);
  } else if (map_impl_type_t == map_impl_type::ROCKS_DB) {
    map_server_ = std::make_shared<RocksDBImpl>(service, kDBPath_server);
  }
  else if (map_impl_type_t == map_impl_type::HCLMAP) {
    MPI_Comm_size(ConfigManager::get_instance()->PROCESS_COMM, &num_clients); // FIXME do we need to transfer the number to non-client processes?
    map_server_ = std::make_shared<HCLMapImpl>(service, true, rank, num_clients);
  }

  if (solver_impl_type_t == solver_impl_type::DP) {
    solver_i = std::make_shared<DPSolver>(service);
  } else if (solver_impl_type_t == solver_impl_type::RANDOM_SELECT) {
    solver_i = std::make_shared<random_solver>(service);
  } else if (solver_impl_type_t == solver_impl_type::ROUND_ROBIN) {
    solver_i = round_robin_solver::getInstance(service);
  } else if (solver_impl_type_t == solver_impl_type::DEFAULT) {
    solver_i = std::make_shared<default_solver>(service);
  }

  switch (service) {
  case LIB: {
    if (rank == 0) {
      auto value =
          map_server()->get(table::SYSTEM_REG, "app_no", std::to_string(-1));
      int curr = 0;
      if (!value.empty()) {
        curr = std::stoi(value);
        curr++;
      }
      application_id = curr;
      map_server()->put(table::SYSTEM_REG, "app_no", std::to_string(curr),
                        std::to_string(-1));
      std::size_t t = map_server()->counter_inc(COUNTER_DB, DATASPACE_ID,
                                                std::to_string(-1));
    }
    MPI_Barrier(ConfigManager::get_instance()->PROCESS_COMM); // MPI_COMM_WORLD
    break;
  }
  case CLIENT: {
    break;
  }
  case SYSTEM_MANAGER: {
    break;
  }
  case TASK_SCHEDULER: {
    std::size_t t = map_server()->counter_inc(COUNTER_DB, ROUND_ROBIN_INDEX,
                                              std::to_string(-1));
    break;
  }
  case WORKER_MANAGER: {
    break;
  }
  case WORKER: {
    break;
  }
  }

  if (map_impl_type_t == map_impl_type::MEMCACHE_D) {
    map_client_ = std::make_shared<MemcacheDImpl>(
        service, ConfigManager::get_instance()->MEMCACHED_URL_CLIENT,
        application_id);
  } else if (map_impl_type_t == map_impl_type::ROCKS_DB) {
    map_client_ = std::make_shared<RocksDBImpl>(service, kDBPath_client);
  }
  else if (map_impl_type_t == map_impl_type::HCLMAP) {
    // for (auto clientnum : hcl_map_client_) {
    //   // FIXME can you really expand the space of a map which is a class member?
    //   hcl_map_client_[clientnum] = std::make_shared<HCLMapImpl>(service, false, clientnum, num_clients)
    // }
  }
}

int dtio_system::build_message_key(MPI_Datatype &message) {
  MPI_Datatype type[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_CHAR};
  int blocklen[4] = {1, 1, 1, KEY_SIZE};
  MPI_Aint disp[4] = {0, sizeof(MPI_INT), 2 * sizeof(MPI_INT),
                      3 * sizeof(MPI_INT)};
  MPI_Type_create_struct(8, blocklen, disp, type, &message);
  MPI_Type_commit(&message);
  return 0;
}

int dtio_system::build_message_file(MPI_Datatype &message_file) {
  MPI_Datatype type[3] = {MPI_CHAR, MPI_INT, MPI_INT};
  int blocklen[3] = {
      KEY_SIZE,
      1,
      1,
  };
  MPI_Aint disp[3] = {0, KEY_SIZE * sizeof(MPI_CHAR),
                      KEY_SIZE * sizeof(MPI_CHAR) + sizeof(MPI_INT)};
  MPI_Type_create_struct(6, blocklen, disp, type, &message_file);
  MPI_Type_commit(&message_file);
  return 0;
}

int dtio_system::build_message_chunk(MPI_Datatype &message_chunk) {
  MPI_Datatype type[5] = {MPI_INT, MPI_INT, MPI_CHAR, MPI_INT, MPI_INT};
  int blocklen[5] = {1, 1, FILE_SIZE, 1, 1};
  MPI_Aint disp[5] = {0, sizeof(MPI_INT), 2 * sizeof(MPI_INT),
                      2 * sizeof(MPI_INT) + FILE_SIZE * sizeof(MPI_CHAR),
                      3 * sizeof(MPI_INT) + FILE_SIZE * sizeof(MPI_CHAR)};
  MPI_Type_create_struct(10, blocklen, disp, type, &message_chunk);
  MPI_Type_commit(&message_chunk);
  return 0;
}
