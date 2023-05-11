#!/bin/bash

mpirun -n 1 ./dtio_task_scheduler ${HOME}/DTIO/conf/default.yaml &
mpirun -n 4 ./dtio_worker ${HOME}/DTIO/conf/default.yaml &
mpirun -n 1 ./dtio_worker_manager ${HOME}/DTIO/conf/default.yaml &
