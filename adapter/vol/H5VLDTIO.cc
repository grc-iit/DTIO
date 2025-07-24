/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Header files needed */
/* Do NOT include private HDF5 files here! */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Public HDF5 file */
#include <hdf5.h>

/* This connector's header */
#include "H5VLDTIO.h"

/**********/
/* Macros */
/**********/

/* Stack allocation size */
#define H5VL_DTIO_SEQ_LIST_LEN 128

/* Definitions for chunking code */
#define H5VL_DTIO_DEFAULT_NUM_SEL_CHUNKS 64
#define H5O_LAYOUT_NDIMS (H5S_MAX_RANK + 1)

/* Whether to display log message when callback is invoked */
/* (Uncomment to enable) */
/* #define ENABLE_DTIO_LOGGING */

/* Hack for missing va_copy() in old Visual Studio editions
 * (from H5win2_defs.h - used on VS2012 and earlier)
 */
#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER < 1800)
#define va_copy(D, S) ((D) = (S))
#endif

/************/
/* Typedefs */
/************/

/* The pass through VOL info object */
typedef struct H5VL_dtio_t {
  int fd; /* fd for the file or dataset */
  char *file_name;
  size_t file_name_len;
  char *dataset_name;
  size_t dataset_name_len;
  unsigned flags;
  hid_t space_id;
  hid_t type_id;
  hid_t dcpl_id;
} H5VL_dtio_t;

/* The pass through VOL wrapper context */
typedef struct H5VL_dtio_wrap_ctx_t {
  hid_t under_vol_id;   /* VOL ID for under VOL */
  void *under_wrap_ctx; /* Object wrapping context for under VOL */
} H5VL_dtio_wrap_ctx_t;

/* Information about a singular selected chunk during a Dataset read/write */
typedef struct H5VL_dtio_select_chunk_info_t {
  uint64_t chunk_coords[H5S_MAX_RANK]; /* The starting coordinates ("upper left
                                          corner") of the chunk */
  hid_t mspace_id;                     /* The memory space corresponding to the
                                          selection in the chunk in memory */
  hid_t fspace_id;                     /* The file space corresponding to the
                                          selection in the chunk in the file */
} H5VL_dtio_select_chunk_info_t;

/* Enum to indicate if the supplied read buffer can be used as a type conversion
 * or background buffer */
typedef enum {
  H5VL_DTIO_TCONV_REUSE_NONE,  /* Cannot reuse buffer */
  H5VL_DTIO_TCONV_REUSE_TCONV, /* Use buffer as type conversion buffer */
  H5VL_DTIO_TCONV_REUSE_BKG    /* Use buffer as background buffer */
} H5VL_dtio_tconv_reuse_t;

/* Udata type for H5Dscatter callback */
typedef struct H5VL_dtio_scatter_cb_ud_t {
  void *buf;
  size_t len;
} H5VL_dtio_scatter_cb_ud_t;

/********************* */
/* Function prototypes */
/********************* */

/* Helper routines */
static H5VL_dtio_t *H5VL_dtio_new_obj(int fd, char *filename, char *dsetname,
                                      unsigned flags, hid_t space_id,
                                      hid_t type_id, hid_t dcpl_id) {}

static herr_t H5VL_dtio_free_obj(H5VL_dtio_t *obj) {}

static herr_t H5VL_dtio_build_io_op_merge(char *filename, char *dsetname,
                                          hid_t mem_space_id,
                                          hid_t file_space_id, size_t type_size,
                                          size_t tot_nelem, void *rbuf,
                                          const void *wbuf) {}

static herr_t H5VL_dtio_build_io_op_match(char *filename, char *dsetname,
                                          hid_t file_space_id, size_t type_size,
                                          size_t tot_nelem, void *rbuf,
                                          const void *wbuf) {}

static herr_t H5VL_dtio_build_io_op_contig(char *filename, char *dsetname,
                                           hid_t file_space_id,
                                           size_t type_size, size_t tot_nelem,
                                           void *rbuf, const void *wbuf) {}

/* "Management" callbacks */
static herr_t H5VL_dtio_init(hid_t vipl_id) {}

static herr_t H5VL_dtio_term(void) {}

/* VOL info callbacks */
static void *H5VL_dtio_info_copy(const void *info) {}

static herr_t H5VL_dtio_info_cmp(int *cmp_value, const void *info1,
                                 const void *info2) {}

static herr_t H5VL_dtio_info_free(void *info) {}

static herr_t H5VL_dtio_info_to_str(const void *info, char **str) {}

static herr_t H5VL_dtio_str_to_info(const char *str, void **info) {}

static htri_t H5VL_dtio_need_bkg(hid_t src_type_id, hid_t dst_type_id,
                                 size_t *dst_type_size, hbool_t *fill_bkg) {}

static herr_t H5VL_dtio_tconv_init(hid_t src_type_id, size_t *src_type_size,
                                   hid_t dst_type_id, size_t *dst_type_size,
                                   hbool_t *_types_equal,
                                   H5VL_dtio_tconv_reuse_t *reuse,
                                   hbool_t *_need_bkg, hbool_t *fill_bkg) {}

static herr_t H5VL_dtio_get_selected_chunk_info(
    hid_t dcpl_id, hid_t file_space_id, hid_t mem_space_id,
    H5VL_dtio_select_chunk_info_t **chunk_info, size_t *chunk_info_len) {}

static herr_t H5VL_dtio_scatter_cb(const void **src_buf,
                                   size_t *src_buf_bytes_used, void *_udata) {}

/* Dataset callbacks */
static void *H5VL_dtio_dataset_create(void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      const char *name, hid_t lcpl_id,
                                      hid_t type_id, hid_t space_id,
                                      hid_t dcpl_id, hid_t dapl_id,
                                      hid_t dxpl_id, void **req) {}

static void *H5VL_dtio_dataset_open(void *obj,
                                    const H5VL_loc_params_t *loc_params,
                                    const char *name, hid_t dapl_id,
                                    hid_t dxpl_id, void **req) {}

static herr_t H5VL_dtio_dataset_read(size_t count, void *dset[],
                                     hid_t mem_type_id[], hid_t mem_space_id[],
                                     hid_t file_space_id[], hid_t plist_id,
                                     void *buf[], void **req) {}

static herr_t H5VL_dtio_dataset_write(size_t count, void *dset[],
                                      hid_t mem_type_id[], hid_t mem_space_id[],
                                      hid_t file_space_id[], hid_t plist_id,
                                      const void *buf[], void **req) {}

static herr_t H5VL_dtio_dataset_get(void *dset, H5VL_dataset_get_args_t *args,
                                    hid_t dxpl_id, void **req) {}

static herr_t H5VL_dtio_dataset_close(void *dset, hid_t dxpl_id, void **req) {}

/* File callbacks */
static void *H5VL_dtio_file_create(const char *name, unsigned flags,
                                   hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id,
                                   void **req) {}

static void *H5VL_dtio_file_open(const char *name, unsigned flags,
                                 hid_t fapl_id, hid_t dxpl_id, void **req) {}

static herr_t H5VL_dtio_file_specific(void *file,
                                      H5VL_file_specific_args_t *args,
                                      hid_t dxpl_id, void **req) {}

static herr_t H5VL_dtio_file_close(void *file, hid_t dxpl_id, void **req) {}

/* Introspection routines */
static herr_t H5VL_dtio_introspect_get_conn_cls(void *obj,
                                                H5VL_get_conn_lvl_t lvl,
                                                const H5VL_class_t **conn_cls) {
}

static herr_t H5VL_dtio_introspect_get_cap_flags(const void *info,
                                                 uint64_t *cap_flags) {}

static herr_t H5VL_dtio_introspect_opt_query(void *obj, H5VL_subclass_t cls,
                                             int opt_type,
                                             uint64_t *supported) {}

/* Pass through VOL connector class struct */
static const H5VL_class_t H5VL_dtio_g = {
    H5VL_VERSION,                                 /* VOL class struct version */
    (H5VL_class_value_t)DTIO_VOL_CONNECTOR_VALUE, /* value        */
    DTIO_VOL_CONNECTOR_NAME,                      /* name         */
    DTIO_VOL_CONNECTOR_VERSION,                   /* connector version */
    0,                                            /* capability flags */
    H5VL_dtio_init,                               /* initialize   */
    H5VL_dtio_term,                               /* terminate    */
    {
        /* info_cls */
        sizeof(H5VL_dtio_info_t), /* size    */
        H5VL_dtio_info_copy,      /* copy    */
        H5VL_dtio_info_cmp,       /* compare */
        H5VL_dtio_info_free,      /* free    */
        H5VL_dtio_info_to_str,    /* to_str  */
        H5VL_dtio_str_to_info     /* from_str */
    },
    {
        /* wrap_cls */
        NULL, /* get_object    */
        NULL, /* get_wrap_ctx  */
        NULL, /* wrap_object   */
        NULL, /* unwrap_object */
        NULL  /* free_wrap_ctx */
    },
    {
        /* attribute_cls */
        NULL, /* create */
        NULL, /* open */
        NULL, /* read */
        NULL, /* write */
        NULL, /* get */
        NULL, /* specific */
        NULL, /* optional */
        NULL, /* close */
    },
    {
        /* dataset_cls */
        H5VL_dtio_dataset_create, /* create */
        H5VL_dtio_dataset_open,   /* open */
        H5VL_dtio_dataset_read,   /* read */
        H5VL_dtio_dataset_write,  /* write */
        NULL,                     // H5VL_dtio_dataset_get,      /* get */
        NULL,                     // H5VL_dtio_dataset_specific, /* specific */
        NULL,                     // H5VL_dtio_dataset_optional, /* optional */
        H5VL_dtio_dataset_close   /* close */
    },
    {
        /* datatype_cls */
        NULL, /* commit */
        NULL, /* open */
        NULL, /* get */
        NULL, /* specific */
        NULL, /* optional */
        NULL, /* close */
    },
    {
        /* file_cls */
        H5VL_dtio_file_create, /* create */
        H5VL_dtio_file_open,   /* open */
        NULL,
        // H5VL_dtio_file_get,      /* get */
        NULL,
        // H5VL_dtio_file_specific, /* specific */
        NULL,
        // H5VL_dtio_file_optional, /* optional */
        H5VL_dtio_file_close /* close */
    },
    {/* group_cls */
     NULL, NULL, NULL, NULL, NULL, NULL},
    {
        /* link_cls */
        NULL, /* create */
        NULL, /* copy */
        NULL, /* move */
        NULL, /* get */
        NULL, /* specific */
        NULL  /* optional */
    },
    {
        /* object_cls */
        NULL, /* open */
        NULL, /* copy */
        NULL, /* get */
        NULL, /* specific */
        NULL, /* optional */
    },
    {
        /* introspect_cls */
        H5VL_dtio_introspect_get_conn_cls,  /* get_conn_cls  */
        H5VL_dtio_introspect_get_cap_flags, /* get_cap_flags */
        H5VL_dtio_introspect_opt_query      /* opt_query     */
    },
    {
        /* request_cls */
        NULL, /* wait         */
        NULL, /* notify       */
        NULL, /* cancel       */
        NULL, /* specific     */
        NULL, /* optional     */
        NULL  /* free         */
    },
    {
        /* blob_cls */
        NULL, /* put          */
        NULL, /* get          */
        NULL, /* specific     */
        NULL  /* optional     */
    },
    {
        /* token_cls */
        NULL, /* cmp          */
        NULL, /* to_str       */
        NULL  /* from_str     */
    },
    NULL /* optional     */
};

/* The connector identification number, initialized at runtime */
static hid_t H5VL_DTIO_g = H5I_INVALID_HID;

/* Error stack declarations */
hid_t H5VL_ERR_STACK_g = H5I_INVALID_HID;
hid_t H5VL_ERR_CLS_g = H5I_INVALID_HID;

H5PL_type_t H5PLget_plugin_type(void) { return H5PL_TYPE_VOL; }
const void *H5PLget_plugin_info(void) { return &H5VL_dtio_g; }
