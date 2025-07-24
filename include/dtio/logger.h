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

#ifndef DTIO_INCLUDE_DTIO_LOGGER_H_
#define DTIO_INCLUDE_DTIO_LOGGER_H_

#include "hermes_shm/util/logging.h"

// Map DTIO logging to HermesShm logging
#define DTIO_LOG_ERROR(...) HELOG(kError, __VA_ARGS__)
#define DTIO_LOG_WARNING(...) HELOG(kWarning, __VA_ARGS__)
#define DTIO_LOG_INFO(...) HELOG(kInfo, __VA_ARGS__)
#define DTIO_LOG_DEBUG(...) HELOG(kDebug, __VA_ARGS__)
#define DTIO_LOG_DEBUG_RANKLESS(...) HELOG(kDebug, __VA_ARGS__)

#endif  // DTIO_INCLUDE_DTIO_LOGGER_H_