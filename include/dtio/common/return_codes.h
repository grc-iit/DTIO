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
#ifndef DTIO_RETURN_CODES_H
#define DTIO_RETURN_CODES_H
/******************************************************************************
 *error code enum
 ******************************************************************************/
typedef enum return_codes {
  SUCCESS = 0,

  /* Error codes*/
  WORKER__SETTING_DIR_FAILED = 7800,
  WORKER__UPDATE_CAPACITY_FAILED = 7801,
  WORKER__UPDATE_SCORE_FAILED = 7802,
  MDM__CREATE_FAILED = 7803,
  MDM__FILENAME_MAX_LENGTH = 7804,
  LIB__FCLOSE_FAILED = 7805,
  MDM__UPDATE_ON_FSEEK_FAILED = 7806,
  UPDATE_FILE_POINTER_FAILED = 7807,
  PREFETCH_ENGINE_FAILED = 7808,
  FILE_SEEK_FAILED = 7809,
  FH_DOES_NOT_EXIST = 7810,
  FILE_READ_FAILED = 7811,
  FILE_WRITE_FAILED = 7812,

  CONTAINER_NOT_VALID = 7821,
  OBJECT_NOT_FOUND = 7822,

  NO_CACHED_DATA_FOUND = 7950
} returncode;

#endif // DTIO_RETURN_CODES_H
