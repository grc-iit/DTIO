#!/bin/bash

mpirun -np 1 ./dtio_worker_manager ${HOME}/DTIO/conf/default.yaml : -np 4 ./dtio_worker ${HOME}/DTIO/conf/default.yaml : -np 1 ./dtio_task_scheduler ${HOME}/DTIO/conf/default.yaml : mpirun -np "$@"
