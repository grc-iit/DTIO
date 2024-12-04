#ifndef DTIO_MAIN_DTIO_H
#define DTIO_MAIN_DTIO_H

#include <cstdio>
#include <cstring>
#include <dtio/common/data_manager/data_manager.h>
#include <dtio/common/metadata_manager/metadata_manager.h>
#include <dtio/drivers/mpi.h>
#include <dtio/dtio_system.h>

// DTIO Namespace
namespace dtio {

int DTIO_Init ();
  int DTIO_write (std::string filename, std::string buf, int64_t offset, size_t count);
  std::string DTIO_read(std::string filename, int64_t offset, size_t count);

}

#endif // DTIO_MAIN_DTIO_H
