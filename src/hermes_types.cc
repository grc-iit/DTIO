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

#include "hermes_types.h"
#include "hermes.h"

namespace hermes {
/** Identifier of the Hermes allocator */
const hipc::allocator_id_t main_alloc_id(0, 1);

/** Hermes server environment variable */
const char* kHermesServerConf = "HERMES_CONF";

/** Hermes client environment variable */
const char* kHermesClientConf = "HERMES_CLIENT_CONF";

/** Hermes adapter mode environment variable */
const char* kHermesAdapterMode = "HERMES_ADAPTER_MODE";

/** Filesystem page size environment variable */
const char* kHermesPageSize = "HERMES_PAGE_SIZE";

/** Stop daemon environment variable */
const char* kHermesStopDaemon = "HERMES_STOP_DAEMON";

/** Maximum path length environment variable */
const size_t kMaxPathLength = 4096;
}  // namespace hermes

namespace hermes::api {

/** Default constructor */
Context::Context()
    : policy(HERMES->server_config_.dpe_.default_policy_),
      rr_split(HERMES->server_config_.dpe_.default_rr_split_),
      rr_retry(false),
      disable_swap(false),
      blob_score_(-1) {}

}  // namespace hermes::api
