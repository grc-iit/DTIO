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

#ifndef DTIO_MAIN_SERIALIZATION_MANAGER_H
#define DTIO_MAIN_SERIALIZATION_MANAGER_H

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <dtio/common/data_structures.h>
#include <sstream>

class serialization_manager {
public:
  // TODO: explore binary with NATS and memcached
  std::string serialize_file_stat(file_stat stat);

  chunk_meta deserialize_chunk(std::string chunk_str);

  std::string serialize_chunk(chunk_meta meta);

  std::string serialize_task(task *task);

  task *deserialize_task(std::string string);
};

#endif // DTIO_MAIN_SERIALIZATION_MANAGER_H
