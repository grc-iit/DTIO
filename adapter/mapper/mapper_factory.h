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

#ifndef HERMES_ADAPTER_FACTORY_H
#define HERMES_ADAPTER_FACTORY_H

#include "abstract_mapper.h"
#include "balanced_mapper.cc"
#include "balanced_mapper.h"
#include "hermes_shm/util/singleton.h"

namespace hermes::adapter {
/**
 A class to represent mapper factory pattern
*/
class MapperFactory {
 public:
  /**
   * Return the instance of mapper given a type. Uses factory pattern.
   *
   * @param[in] type type of mapper to be used by the POSIX adapter.
   * @return Instance of mapper given a type.
   */
  AbstractMapper* Get(const MapperType& type) {
    switch (type) {
      case MapperType::kBalancedMapper: {
        return hshm::EasySingleton<BalancedMapper>::GetInstance();
      }
      default: {
        // TODO(llogan): @error_handling Mapper not implemented
      }
    }
    return NULL;
  }
};
}  // namespace hermes::adapter
#endif  // HERMES_ADAPTER_FACTORY_H
