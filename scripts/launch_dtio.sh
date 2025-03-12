#!/bin/bash

# *
# * Copyright (C) 2024 Gnosis Research Center <grc@iit.edu>, 
# * Keith Bateman <kbateman@hawk.iit.edu>, Neeraj Rajesh
# * <nrajesh@hawk.iit.edu> Hariharan Devarajan
# * <hdevarajan@hawk.iit.edu>, Anthony Kougkas <akougkas@iit.edu>,
# * Xian-He Sun <sun@iit.edu>
# *
# * This file is part of DTIO
# *
# * DTIO is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as
# * published by the Free Software Foundation, either version 3 of the
# * License, or (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU Lesser General Public License for more details.
# *
# * You should have received a copy of the GNU General Public
# * License along with this program.  If not, see
# * <http://www.gnu.org/licenses/>.
# *

set -x
CONF=${HOME}/DTIO/conf/default.yaml
NUM_WORKERS=`grep NUM_WORKERS  ${CONF} | awk '{print $2}'`
mpirun -np ${NUM_WORKERS} ./dtio_worker ${CONF} : -np 1 ./dtio_worker_manager ${CONF} : -np 1 ./dtio_task_scheduler ${CONF} : -np 9 ./dtio_hclmanager ${CONF}
rm -vf /dev/shm/{dataspace,metadata,metadata+*,taskscheduler+TASK+,worker+[0-9]+}_0
