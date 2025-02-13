#include "dtio/common/config_manager.h"
#include "dtio/common/enumerations.h"
#include "dtio/common/logger.h"
#include <dtio/common/utilities.h>
#include <dtio/drivers/dtio.h>
#include <iostream>
#include <nanobind/nanobind.h>
// #include <nanobind/ndarray.h>

int dtio::DTIO_Init()
{
  int provided;
  PMPI_Init_thread (NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
  if (provided < MPI_THREAD_MULTIPLE)
    {
      std::cerr << "Didn't receive appropriate thread specification"
                << std::endl;
    }

  /* Note: The following log statement was initially at the beginning
     of initialization, but DTIO logging requires rank so we're going
     to have to wait to log until after MPI has initialized.
   */
  DTIO_LOG_DEBUG("[MPI] Init Entered");

  /* NOTE: If we're intercepting MPI_Init, we can't assume that
     argv[1] is the DTIO conf path
   */
  ConfigManager::get_instance ()->LoadConfig ();

  dtio_system::getInstance (service::LIB);
  DTIO_LOG_DEBUG ("[MPI] Comm: Complete");
  return 0;
}

int dtio::DTIO_write (std::string filename, std::string buf, int64_t offset, size_t count)
{
  DTIO_LOG_DEBUG ("[POSIX] Write Entered");
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto map_client = dtio_system::getInstance (LIB)->map_client ();
  auto map_server = dtio_system::getInstance (LIB)->map_server ();
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance (LIB);
  // auto filename = mdm->get_filename (fd);
  // auto offset = mdm->get_fp (filename);

  if (!mdm->is_opened (filename))
    throw std::runtime_error ("dtio::write() file not opened!");
  auto source = file();
  source.offset = 0; // buffer offset, NOT file offset
  source.size = count;
  auto destination = file(filename, offset, count);
  auto w_task
      = task (task_type::WRITE_TASK, source, destination);

  if (ConfigManager::get_instance()->USE_URING) {
    w_task.iface = io_client_type::URING;
  }
  else {
    w_task.iface = io_client_type::POSIX;
  }
#ifdef TIMERTB
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  auto write_tasks
    = task_m->build_write_task (w_task, buf.data());
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_write_task(),"
          << t.pauseTime () << "\n";
  DTIO_LOG_TRACE(stream1.str ());
#endif

  const char *write_data_char = buf.data();
  std::string write_data (buf.data(), count);

  int index = 0;
  /* Note: We cannot assume buf is null-terminated. It would likely be
   * better to get rid of the string entirely, but for now if we copy
   * with count then it will not assume a null-terminated C string */
  std::vector<std::pair<std::string, std::string> > task_ids
      = std::vector<std::pair<std::string, std::string> > ();

  for (auto tsk : write_tasks)
    {
      if (tsk->addDataspace)
	{
	  if (write_data.length () >= tsk->source.offset + tsk->source.size)
	    {
	      auto data
		= write_data.substr (tsk->source.offset, tsk->source.size);
	      DTIO_LOG_TRACE("Write v1 " << write_data.length() << " " << data.length() << " " << tsk->source.offset << " " << tsk->source.size << std::endl);
	      DTIO_LOG_TRACE("WRITE Count " << count << std::endl);
	      data_m->put (DATASPACE_DB, tsk->source.filename, write_data_char, count,
			   std::to_string (tsk->destination.server));

	      // data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
	      //              std::to_string (tsk->destination.server));
	    }
	  else
	    {
	      DTIO_LOG_TRACE("Write v2 " << write_data.length() << " " << tsk->source.size << " " << write_tasks.size() << std::endl);
	      // data_m->put (DATASPACE_DB, tsk->source.filename, write_data,
	      //              std::to_string (tsk->destination.server));
	      DTIO_LOG_TRACE("WRITE Count " << count << std::endl);
	      data_m->put (DATASPACE_DB, tsk->source.filename, write_data_char, count,
			   std::to_string (tsk->destination.server));
	    }
	}

      if (tsk->publish)
        {
          if (count < tsk->source.size) {
            mdm->update_write_task_info (*tsk, tsk->destination.filename, count);
	  }
          else {
            mdm->update_write_task_info (*tsk, tsk->destination.filename, tsk->source.size);
	  }
          client_queue->publish_task (tsk);

          task_ids.emplace_back (std::make_pair (
						 std::to_string(tsk->task_id), std::to_string (tsk->destination.server)));
        }
      else
        {
          mdm->update_write_task_info (*tsk, tsk->destination.filename, tsk->source.size);
        }

      index++;
      delete tsk;
    }
  // NOTE Without this, it's not synchronous. Easy way to add async later
  if (!ConfigManager::get_instance ()->ASYNC) {
    for (auto task_id : task_ids)
      {
	while (!map_server->exists (table::WRITE_FINISHED_DB, task_id.first,
				    std::to_string (-1)))
	  {
	  }
	map_server->remove (table::WRITE_FINISHED_DB, task_id.first,
			    std::to_string (-1));
	// map_client->remove (table::DATASPACE_DB, task_id.first, task_id.second);
      }
  }

  return count;
}

std::string dtio::DTIO_read(std::string filename, int64_t offset, size_t count)
{
  auto mdm = metadata_manager::getInstance (LIB);
  auto client_queue
      = dtio_system::getInstance (LIB)->get_client_queue (CLIENT_TASK_SUBJECT);
  auto task_m = dtio_system::getInstance(LIB)->task_composer();
  auto data_m = data_manager::getInstance (LIB);
  // auto filename = mdm->get_filename (fd);
  // auto offset = mdm->get_fp (filename);
  bool check_fs = false;
  task *task_i;
  std::string buf;
  buf.reserve(count);
  task_i = nullptr;

  if (!mdm->is_opened (filename))
    {
      if (!ConfigManager::get_instance ()->CHECKFS)
        {
          return buf;
        }
      if (!mdm->is_created (filename))
        {
          check_fs = true;
        }
    }
  auto r_task
      = task (task_type::READ_TASK, file (filename, offset, count), file ());
  r_task.check_fs = check_fs;
  if (ConfigManager::get_instance()->USE_URING) {
    r_task.iface = io_client_type::URING;
  }
  else {
    r_task.iface = io_client_type::POSIX;
  }
#ifdef TIMERTB
  hcl::Timer t = hcl::Timer ();
  t.resumeTime ();
#endif
  auto tasks = task_m->build_read_task (r_task);
  DTIO_LOG_TRACE("Task len " << tasks.size() << std::endl);
#ifdef TIMERTB
  std::stringstream stream1;
  stream1 << "build_read_task(),"
          << t.pauseTime () << "\n";
  DTIO_LOG_TRACE(stream1.str ());
#endif
  int ptr_pos = 0;
  size_t size_read = 0;
  task *ttp;
  for (auto t : tasks)
    {
      std::string data;
#if(STACK_ALLOCATION)
      {
	// char char_data[MAX_IO_UNIT];
	char *char_data;
	char_data = (char *)malloc(MAX_IO_UNIT);
	switch (t.source.location)
	  {
	  case PFS:
	    {
	      DTIO_LOG_DEBUG ("in pfs");
	      if (ConfigManager::get_instance ()->CHECKFS)
		{
		  hcl::Timer timer = hcl::Timer ();
		  client_queue->publish_task (&t);

		  while (!data_m->exists (DATASPACE_DB, t.source.filename,
					  std::to_string (t.destination.server)))
		    {
		    }

		  data_m->get (DATASPACE_DB, t.source.filename,
			       std::to_string (t.destination.server), char_data);

		  strncpy (buf.data() + ptr_pos,
			   char_data + t.source.offset, t.destination.size);
		  auto print_test = std::string (buf.data());

		  data_m->remove (DATASPACE_DB, t.source.filename,
				  std::to_string (t.destination.server));

		  size_read += t.source.size;
		}
	      else
		{
		  DTIO_LOG_ERROR ("in pfs no checkfs");
		}
	    }
	    break;
	  case BUFFERS:
	    {
	      hcl::Timer timer = hcl::Timer ();
	      client_queue->publish_task (&t);
	      while (!data_m->exists (DATASPACE_DB, t.source.filename,
				      std::to_string (t.destination.server)))
		{
		  // std::cerr<<"looping\n";
		}
	      data_m->get (DATASPACE_DB, t.source.filename,
			   std::to_string (t.destination.server), char_data);

	      strncpy (buf.data() + ptr_pos, char_data + t.source.offset,
		       t.destination.size);

	      data_m->remove (DATASPACE_DB, t.source.filename,
			      std::to_string (t.destination.server));

	      size_read += t.destination.size;
	      break;
	    }
	  case CACHE:
	    {
	      DTIO_LOG_TRACE("Cache" << std::endl);
	      // data = data_m->get (DATASPACE_DB, t.source.filename,
	      //                     std::to_string (t.destination.server));
	      data_m->get(DATASPACE_DB, t.source.filename,
			  std::to_string (t.destination.server), char_data);
	      DTIO_LOG_TRACE("Doing memcpy " << ptr_pos << " " << t.source.offset << " " << t.destination.size << " " << t.source.size << std::endl);
	      memcpy (buf.data() + ptr_pos, char_data + (t.source.offset % t.source.size),
		      t.destination.size); // Might not actually be necessary
	      DTIO_LOG_TRACE("Memcpy finished" << std::endl);
	      size_read += t.destination.size;
	      break;
	    }
	  }
	free(char_data);
      }
#else
      {
	char *char_data = (char *)malloc(MAX_IO_UNIT);
	switch (t.source.location)
	  {
	  case PFS:
	    {
	      DTIO_LOG_DEBUG ("in pfs");
	      if (ConfigManager::get_instance ()->CHECKFS)
		{
		  hcl::Timer timer = hcl::Timer ();
		  client_queue->publish_task (&t);

		  while (!data_m->exists (DATASPACE_DB, t.source.filename,
					  std::to_string (t.destination.server)))
		    {
		    }
		  std::cout << "Raw data get" << std::endl;

		  data_m->get (DATASPACE_DB, t.source.filename,
			       std::to_string (t.destination.server), char_data);

		  std::cout << "Get data into " << ptr_pos << " from " << t.source.offset << " size " << t.destination.size << std::endl;
		  strncpy (buf.data() + ptr_pos,
			   char_data, t.destination.size); //  + t.source.offset
		  std::cout << "Success" << std::endl;
		  free(char_data);
		  // auto print_test = std::string ((char *)buf);

		  data_m->remove (DATASPACE_DB, t.source.filename,
				  std::to_string (t.destination.server));

		  size_read += t.source.size;
		}
	      else
		{
		  DTIO_LOG_DEBUG ("in pfs no checkfs");
		}
	    }
	    break;
	  case BUFFERS:
	    {
	      hcl::Timer timer = hcl::Timer ();
	      client_queue->publish_task (&t);
	      while (!data_m->exists (DATASPACE_DB, t.source.filename,
				      std::to_string (t.destination.server)))
		{
		  // std::cerr<<"looping\n";
		}
	      std::cout << "Raw data get" << std::endl;
	      data_m->get (DATASPACE_DB, t.source.filename,
			   std::to_string (t.destination.server), char_data);

	      std::cout << "Get data into " << ptr_pos << " from " << t.source.offset << " size " << t.destination.size << std::endl;

	      strncpy (buf.data() + ptr_pos, char_data,
		       t.destination.size); // + t.source.offset

	      std::cout << "Success" << std::endl;

	      free(char_data);
	      data_m->remove (DATASPACE_DB, t.source.filename,
			      std::to_string (t.destination.server));

	      size_read += t.destination.size;
	      break;
	    }
	  case CACHE:
	    {
	      DTIO_LOG_TRACE("Cache" << std::endl);
	      // data = data_m->get (DATASPACE_DB, t.source.filename,
	      //                     std::to_string (t.destination.server));
	      data_m->get(DATASPACE_DB, t.source.filename,
			  std::to_string (t.destination.server), char_data);
	      DTIO_LOG_TRACE("Doing memcpy " << ptr_pos << " " << t.source.offset << " " << t.destination.size << " " << t.source.size << std::endl);
	      memcpy (buf.data() + ptr_pos, char_data + (t.source.offset % t.source.size),
		      t.destination.size); // Might not actually be necessary
	      DTIO_LOG_TRACE("Memcpy finished" << std::endl);
	      free(char_data);
	      size_read += t.destination.size;
	      break;
	    }
	  }
      }
#endif


      ptr_pos += t.destination.size;
    }
  mdm->update_read_task_info (tasks, filename);
  return buf;
}

NB_MODULE(dtiopy, m) {
  m.def("DTIO_Init", &dtio::DTIO_Init);
  m.def("DTIO_write", &dtio::DTIO_write);
  m.def("DTIO_read", &dtio::DTIO_read);
}
