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
#ifndef DTIO_MAIN_SYSTEM_MANAGER_SERVICE_H
#define DTIO_MAIN_SYSTEM_MANAGER_SERVICE_H

#include <dtio/common/enumerations.h>
#include <memory>

class system_manager_service {
  static std::shared_ptr<system_manager_service> instance;
  service service_i;
  explicit system_manager_service(service service)
      : service_i(service), kill(false) {}
  int check_applications_score();

public:
  int kill;

  inline static std::shared_ptr<system_manager_service>
  getInstance(service service) {
    return instance == nullptr
               ? instance = std::shared_ptr<system_manager_service>(
                     new system_manager_service(service))
               : instance;
  }
  void run();
};

#endif // DTIO_MAIN_SYSTEM_MANAGER_SERVICE_H
