#!/bin/bash

mpirun -np 4 ./dtio_worker ${HOME}/DTIO/conf/default.yaml : -np 1 ./dtio_worker_manager ${HOME}/DTIO/conf/default.yaml : -np 1 ./dtio_task_scheduler ${HOME}/DTIO/conf/default.yaml : -np 2 ./dtio_hclmap ${HOME}/DTIO/conf/default.yaml
rm -vf /dev/shm/{dataspace,metadata,taskscheduler+TASK+,worker+[0-9]+}_0
