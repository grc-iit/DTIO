/*
 * Copyright (C) 2019  SCS Lab <scs-help@cs.iit.edu>, Hariharan
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
/*
 * Created by hariharan on 2/16/18.
 * Updated by kbateman and nrajesh (nrj5k) since
 */

#include "client.h"
#include <cstring>
#include <dtio/common/return_codes.h>
#include <dtio/dtio_system.h>
#include <mpi.h>

int
DTIOClient::init ()
{
  MPI_Open_port (MPI_INFO_NULL,
                 const_cast<char *> (DTIO_CLIENT_PORT.c_str ()));
  MPI_Comm_rank (MPI_COMM_SELF, &rank);
  MPI_Intercomm_merge (MPI_COMM_SELF, 0, &applications_comms);
  return SUCCESS;
}

int
DTIOClient::listen_application_connections ()
{
  while (true)
    {
      MPI_Comm application_comm;
      MPI_Comm_accept (DTIO_CLIENT_PORT.c_str (), MPI_INFO_NULL, rank,
                       MPI_COMM_SELF, &application_comm);
      int client_rank;
      MPI_Comm_rank (applications_comms, &client_rank);
      MPI_Ssend (&client_rank, 1, MPI_INT, 1, MPI_ANY_TAG, application_comm);
      count++;
      application_map.emplace (count, application_comm);
      MPI_Intercomm_merge (application_comm, 0, &applications_comms);
      if (!async_handle.valid ())
        {
          async_handle
              = std::async (std::launch::async,
                            &DTIOClient::initialize_application, this, count);
        }
    }
}

int
DTIOClient::initialize_application (size_t application_id)
{
  return 0;
}

int
DTIOClient::listen_request ()
{
  message msg;
  message_key key;
  MPI_Datatype message_key;
  int error
      = dtio_system::getInstance (CLIENT)->build_message_key (message_key);
  while (true)
    {
      MPI_Status status;
      MPI_Recv (&key, 1, message_key, MPI_ANY_SOURCE, MPI_ANY_TAG,
                applications_comms, &status);
      int source = status.MPI_SOURCE;
      switch (key.m_type)
        {
        case METADATA:
          {
            switch (key.mp_type)
              {
              case META_FH:
                {
                  file file_struct;
                  MPI_Datatype message_file;
                  error
                      = dtio_system::getInstance (CLIENT)->build_message_file (
                          message_file);
                  MPI_Status status_file;
                  MPI_Recv (&file_struct, 1, message_file, source, MPI_ANY_TAG,
                            applications_comms, &status_file);
                  file_meta f;
                  f.file_struct.size = file_struct.size;
                  f.file_struct.offset = file_struct.offset;
                  strcpy (const_cast<char *> (f.file_struct.filename.c_str ()),
                          file_struct.filename.c_str ());
                  switch (key.operation_type)
                    {
                    case WRITE:
                      {
                        update_file (f, key.key);
                        break;
                      }
                    case READ:
                      {
                        get_file (f, key.key);
                        file_struct = f.file_struct;
                        MPI_Ssend (&file_struct, 1, message_file, source, 0,
                                   applications_comms);
                        break;
                      }
                    case DELETE:
                      {
                        delete_file (f, key.key);
                        break;
                      }
                    }
                  break;
                }
              case META_CHUNK:
                {
                  chunk_msg chunk_struct;
                  MPI_Datatype message_chunk;
                  error = dtio_system::getInstance (CLIENT)
                              ->build_message_chunk (message_chunk);
                  MPI_Status status_file;
                  MPI_Recv (&chunk_struct, 1, message_chunk, source,
                            MPI_ANY_TAG, applications_comms, &status_file);
                  file_meta f;
                  f.file_struct.size = chunk_struct.file_size;
                  f.file_struct.offset = chunk_struct.offset;
                  strcpy (const_cast<char *> (f.file_struct.filename.c_str ()),
                          chunk_struct.filename.c_str ());
                  chunk_meta c;
                  c.destination.size = chunk_struct.file_size;
                  c.destination.offset = chunk_struct.offset;
                  if (chunk_struct.dataspace_id.empty ())
                    {
                      strcpy (
                          const_cast<char *> (c.destination.filename.c_str ()),
                          chunk_struct.filename.c_str ());
                    }
                  else
                    {
                      c.destination.filename = chunk_struct.dataspace_id;
                    }
                  f.chunks.push_back (c);
                  switch (key.operation_type)
                    {
                    case WRITE:
                      {
                        update_chunk (f, key.key);
                        break;
                      }
                    case READ:
                      {
                        get_chunk (f, key.key);
                        chunk_msg msgs[f.chunks.size ()];
                        for (int i = 0; i < f.chunks.size (); ++i)
                          {
                            msgs[i].chunkType
                                = f.chunks[i].destination.location;
                            msgs[i].dataspace_id
                                = f.chunks[i].destination.filename;
                            strcpy (
                                const_cast<char *> (msgs[i].filename.c_str ()),
                                f.chunks[i].destination.filename.c_str ());
                            msgs[i].offset = f.chunks[i].destination.offset;
                            msgs[i].file_size = f.chunks[i].destination.size;
                          }
                        MPI_Ssend (&msgs, f.chunks.size (), message_chunk,
                                   source, 0, applications_comms);
                        break;
                      }
                    case DELETE:
                      {
                        delete_chunk (f, key.key);
                        break;
                      }
                    }

                  break;
                }
              }
          }
        case DATASPACE:
          {
            MPI_Probe (source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int char_count;
            MPI_Get_count (&status, MPI_CHAR, &char_count);
            char *data = static_cast<char *> (malloc (char_count));
            MPI_Recv (data, char_count, MPI_CHAR, source, MPI_ANY_TAG,
                      applications_comms, &status);
            break;
          }
        }
    }
  return 0;
}

int
DTIOClient::update_file (file_meta f, std::string key)
{
  return 0;
}

int
DTIOClient::update_dataspace (size_t id, dataspace data)
{
  return 0;
}

int
DTIOClient::get_file (file_meta &f, std::string key)
{
  return 0;
}

int
DTIOClient::get_dataspace (size_t id, dataspace &data)
{
  return 0;
}

int
DTIOClient::get_chunk (file_meta &f, std::string key)
{
  return 0;
}

int
DTIOClient::delete_file (file_meta &f, std::string key)
{
  return 0;
}

int
DTIOClient::delete_chunk (file_meta &f, std::string key)
{
  return 0;
}

int
DTIOClient::update_chunk (file_meta f, std::string key)
{
  return 0;
}
