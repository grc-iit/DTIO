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
/*******************************************************************************
 * Created by hariharan on 5/8/18.
 * Updated by akougkas on 6/30/2018
 ******************************************************************************/
#ifndef DTIO_MAIN_SOLVER_H
#define DTIO_MAIN_SOLVER_H
/******************************************************************************
 *include files
 ******************************************************************************/
#include <dtio/common/data_structures.h>
/******************************************************************************
 *Class
 ******************************************************************************/
class solver {
protected:
  /******************************************************************************
   *Variables and members
   ******************************************************************************/
  service service_i;

public:
  /******************************************************************************
   *Constructor
   ******************************************************************************/
  explicit solver(service service) : service_i(service) {}
  /******************************************************************************
   *Interface
   ******************************************************************************/
  virtual solver_output solve(solver_input input) = 0;
  /******************************************************************************
   *Destructor
   ******************************************************************************/
  virtual ~solver() {}
};
#endif // DTIO_MAIN_SOLVER_H
