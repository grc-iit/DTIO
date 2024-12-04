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
/******************************************************************************
 *include files
 ******************************************************************************/
#include <dtio/common/data_structures.h>
#include <dtio/common/external_clients/hcl_map_impl.h>
#include <dtio/common/external_clients/hcl_queue_impl.h>
#include <dtio/common/logger.h>
#include <dtio/common/utilities.h>
#include <mpi.h>
/******************************************************************************
 *Main
 ******************************************************************************/
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
  // Assumption: one worker manager
  MPI_Comm_split (MPI_COMM_WORLD, HCLMAP_COLOR,
                  rank - ConfigManager::get_instance ()->NUM_WORKERS - 1
                      - ConfigManager::get_instance ()->NUM_SCHEDULERS,
                  &ConfigManager::get_instance ()->PROCESS_COMM);
  int process_rank;
  MPI_Comm_rank (ConfigManager::get_instance ()->PROCESS_COMM, &process_rank);
  if (process_rank == 0)
    {
      MPI_Comm_split (MPI_COMM_WORLD, DATASPACE_COLOR, rank - 1,
                      &ConfigManager::get_instance ()->DATASPACE_COMM);
    }
  else {
    MPI_Comm_split (MPI_COMM_WORLD, DATASPACE_NULL_COLOR, process_rank,
		    &ConfigManager::get_instance ()->DATASPACE_COMM);
  }
  if (process_rank == 1 || process_rank == 2 || process_rank == 3)
    {
      MPI_Comm_split (MPI_COMM_WORLD, METADATA_COLOR, rank - 2,
                      &ConfigManager::get_instance ()->METADATA_COMM);
    }
  else {
    MPI_Comm_split (MPI_COMM_WORLD, METADATA_NULL_COLOR, process_rank == 0 ? 1 : process_rank,
		    &ConfigManager::get_instance ()->METADATA_COMM);
  }
  for (int i = 0; i < ConfigManager::get_instance ()->NUM_WORKERS; i++)
    {
      if (process_rank == 0 || process_rank == 1 || process_rank == 2 || process_rank == 3) {
	MPI_Comm_split (MPI_COMM_WORLD, QUEUE_WORKER_NULL_COLOR,
			process_rank + ConfigManager::get_instance()->NUM_WORKERS + ConfigManager::get_instance()->NUM_SCHEDULERS,
			&ConfigManager::get_instance ()->QUEUE_WORKER_COMM[i]);
      }
      else if (i == process_rank - 5) {
	MPI_Comm_split (MPI_COMM_WORLD, QUEUE_WORKER_COLOR, 0,
                      &ConfigManager::get_instance ()->QUEUE_WORKER_COMM[i]);
      }
      else {
	MPI_Comm_split (MPI_COMM_WORLD, QUEUE_WORKER_NULL_COLOR,
			((process_rank - 5) < i ? (ConfigManager::get_instance()->NUM_WORKERS + ConfigManager::get_instance()->NUM_SCHEDULERS + process_rank - 2) : (ConfigManager::get_instance()->NUM_WORKERS + ConfigManager::get_instance()->NUM_SCHEDULERS + process_rank - 3)),
			&ConfigManager::get_instance ()->QUEUE_WORKER_COMM[i]);
      }
    }
  if (process_rank == 0 || process_rank == 1 || process_rank == 2 || process_rank == 3)
    {
      MPI_Comm_split (MPI_COMM_WORLD, QUEUE_TS_NULL_COLOR, rank - 1,
                      &ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM);
    }
  else if (process_rank == 4)
    {
      MPI_Comm_split (MPI_COMM_WORLD, QUEUE_TS_COLOR, 0,
		      &ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM);
    }
  else
    {
      MPI_Comm_split (MPI_COMM_WORLD, QUEUE_TS_NULL_COLOR, rank - 2,
                      &ConfigManager::get_instance ()->QUEUE_TASKSCHED_COMM);
    }
  DTIO_LOG_INFO ("[HCL Manager] Creating");
  std::shared_ptr<hcl::HCL> hcl_init;
  hcl_init = nullptr;
  std::shared_ptr<HCLMapImpl> map;
  std::shared_ptr<HCLQueueImpl> queue;
  if (process_rank == 0)
    {
      map = std::make_shared<HCLMapImpl> (HCLCLIENT, "dataspace", 0, 1, hcl_init);
    }
  else if (process_rank == 1)
    {
      map = std::make_shared<HCLMapImpl> (HCLCLIENT, "metadata", 0, 1, hcl_init);
    }
  else if (process_rank == 2) {
    map = std::make_shared<HCLMapImpl> (HCLCLIENT, "metadata+fs", 0, 1, hcl_init);
  }
  else if (process_rank == 3) {
    map = std::make_shared<HCLMapImpl> (HCLCLIENT, "metadata+filemeta", 0, 1, hcl_init);
  }
  else if (process_rank == 4) {
    // was 2 originally
    queue = std::make_shared<HCLQueueImpl> (HCLCLIENT, CLIENT_TASK_SUBJECT, 0, 1, false, hcl_init);
  }
  else {
    queue = std::make_shared<HCLQueueImpl> (HCLCLIENT, std::to_string(process_rank - 5), 0, 1, false, hcl_init);
  }
  DTIO_LOG_INFO ("[HCL Manager] Created");
  // std::cout << "HCL Manager rank " << process_rank << " pid " << getpid() << std::endl;
  // if (process_rank > 3) {
  //   while (true) {
  //     uint16_t key = 0;
  //     if (queue->subscribe_task_helper()) {
  // 	std::cout << "Element received rank " << process_rank << std::endl;
  // 	auto tsk = queue->subscribe_task_getter();
  // 	if (tsk != nullptr) {
  // 	  std::cout << "Received task " << tsk->task_id << std::endl;
  // 	}
  //     }
  //   }
  // }
  // else {
  while (true); // Busy loop
  // }
  // map->run ();

  hcl_init->Finalize();
  MPI_Finalize ();
  return 0;
}
