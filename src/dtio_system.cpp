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

#include "dtio/config_manager.h"
#include "dtio/constants.h"
#include "dtio/enumerations.h"
#include <dtio/dtio_system.h>

std::shared_ptr<dtio_system> dtio_system::instance = nullptr;

// Interface
void
dtio_system::init (service service)
{
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  int comm_size;
  MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
  comm_size = comm_size == 0 ? 1 : comm_size;

  printf ("Get client queue\n");
  if (service == LIB)
    {
      // get_client_queue functionality removed
    }

  printf ("Init IOWarp DS map\n");
  if (map_impl_type_t == map_impl_type::IOWARP)
    {
      map_server_ = std::make_shared<IOWARPMapImpl> (service, "dataspace");
    }

  switch (service)
    {
    case LIB:
      {
        if (rank == 0)
          {
            printf ("Init lib get\n");
            auto value = map_server ()->get (table::SYSTEM_REG, "app_no",
                                             std::to_string (-1));
            int curr = 0;
            if (!value.empty ())
              {
                curr = std::stoi (value);
                curr++;
              }
            application_id = curr;
            printf ("Init lib put\n");
            map_server ()->put (table::SYSTEM_REG, "app_no",
                                std::to_string (curr), std::to_string (-1));
            printf ("Init lib counter increment\n");
            std::size_t t = map_server ()->counter_inc (
                COUNTER_DB, DATASPACE_ID, std::to_string (-1));
            printf ("Init lib counter increment done\n");
          }
        MPI_Barrier (MPI_COMM_WORLD);
        break;
      }
    case CLIENT:
      {
        break;
      }
    case SYSTEM_MANAGER:
      {
        break;
      }
    case TASK_SCHEDULER:
      {
        map_server ()->counter_inc (COUNTER_DB, ROUND_ROBIN_INDEX,
                                    std::to_string (-1));
        break;
      }
    case WORKER_MANAGER:
      {
        break;
      }
    case WORKER:
      {
        break;
      }
    }

  // if (map_impl_type_t == map_impl_type::MEMCACHE_D)
  //   {
  //     map_client_ = std::make_shared<MemcacheDImpl> (
  //         service, ConfigManager::get_instance ()->MEMCACHED_URL_CLIENT,
  //         application_id);
  //   }
  // else if (map_impl_type_t == map_impl_type::ROCKS_DB)
  //   {
  //     map_client_ = std::make_shared<RocksDBImpl> (service, kDBPath_client);
  //   }
  // else
  printf ("Init IOWarp clients\n");
  if (map_impl_type_t == map_impl_type::IOWARP)
    {
      map_client_ = std::make_shared<IOWARPMapImpl> (service, "metadata");
      fs_map_ = std::make_shared<IOWARPMapImpl> (service, "metadata+fs");
      // cm_map_
      // 	= std::make_shared<HCLMapImpl> (service, "metadata+chunkmeta",
      // 0, 1, hcl_init);
      fm_map_ = std::make_shared<IOWARPMapImpl> (service, "metadata+filemeta");
    }
  printf ("Init done?\n");
}
