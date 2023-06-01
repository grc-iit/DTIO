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
#include <dtio/common/external_clients/hcl_queue_impl.h>
#include <mpi.h>

int HCLQueueImpl::publish_task(task *task_t) {
}

task *HCLQueueImpl::subscribe_task_with_timeout(int &status) {
}

task *HCLQueueImpl::subscribe_task(int &status) {
}

int HCLQueueImpl::get_queue_size() {
}

int HCLQueueImpl::get_queue_count() {
}

int HCLQueueImpl::get_queue_count_limit() { }
