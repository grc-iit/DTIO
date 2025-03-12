/*
 * Purpose:	The private header file for the DTIO VOL plugin.
 */
#ifndef H5VLDTIO_H
#define H5VLDTIO_H

#include <mpi.h>

/* Public headers needed by this file */
#include <H5VLpublic.h> /* Virtual Object Layer                 */
#include <H5public.h>   /* Generic Functions                    */
#include <H5Ipublic.h>  /* IDs                                  */
#include <H5PLextern.h>

/* Semi-public headers mainly for VOL connector authors */
#include "H5VLerror.h"
#include <H5VLconnector.h>

#ifdef __cplusplus
extern "C" {
#endif

#define H5VL_DTIO (H5VL_dtio_register())

#define DTIO_VOL_CONNECTOR_VALUE ((H5VL_class_value_t)721)
#define DTIO_VOL_CONNECTOR_NAME "dtio_vol_connector"
#define DTIO_VOL_CONNECTOR_VERSION 0

#define DTIO_HDF5_FILENAME_MAX 1024

/* DTIO-specific file access properties */
typedef struct H5VL_dtio_info_t {
  char filename[DTIO_HDF5_FILENAME_MAX];
} H5VL_dtio_info_t;


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Nothing */
H5_DLL hid_t H5VL_dtio_register(void);

#ifdef __cplusplus
}
#endif

#endif /* H5VLDTIO_H */
