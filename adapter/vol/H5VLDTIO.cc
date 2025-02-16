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

/* DTIO headers */
#include <dtio/drivers/hdf5.h>

/* This connector's header */
#include "H5VLDTIO.h"

/**********/
/* Macros */
/**********/

/* Stack allocation size */
#define H5VL_DTIO_SEQ_LIST_LEN         128

/* Definitions for chunking code */
#define H5VL_DTIO_DEFAULT_NUM_SEL_CHUNKS   64
#define H5O_LAYOUT_NDIMS                    (H5S_MAX_RANK+1)

/* Whether to display log message when callback is invoked */
/* (Uncomment to enable) */
/* #define ENABLE_PASSTHRU_LOGGING */

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
    uint64_t chunk_coords[H5S_MAX_RANK]; /* The starting coordinates ("upper left corner") of the chunk */
    hid_t    mspace_id;                  /* The memory space corresponding to the
                                            selection in the chunk in memory */
    hid_t    fspace_id;                  /* The file space corresponding to the
                                            selection in the chunk in the file */
} H5VL_dtio_select_chunk_info_t;

/* Enum to indicate if the supplied read buffer can be used as a type conversion
 * or background buffer */
typedef enum {
    H5VL_DTIO_TCONV_REUSE_NONE,    /* Cannot reuse buffer */
    H5VL_DTIO_TCONV_REUSE_TCONV,   /* Use buffer as type conversion buffer */
    H5VL_DTIO_TCONV_REUSE_BKG      /* Use buffer as background buffer */
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
static H5VL_dtio_t *H5VL_dtio_new_obj(int fd, char *filename, char *dsetname, unsigned flags, hid_t space_id, hid_t type_id, hid_t dcpl_id);
static herr_t               H5VL_dtio_free_obj(H5VL_dtio_t *obj);
static herr_t H5VL_dtio_build_io_op_merge(char *filename, char *dsetname, hid_t mem_space_id, hid_t file_space_id,
					  size_t type_size, size_t tot_nelem,
					  void *rbuf, const void *wbuf);
static herr_t H5VL_dtio_build_io_op_match(char *filename, char *dsetname, hid_t file_space_id, size_t type_size,
					  size_t tot_nelem, void *rbuf, const void *wbuf);
static herr_t H5VL_dtio_build_io_op_contig(char *filename, char *dsetname, hid_t file_space_id, size_t type_size,
					    size_t tot_nelem, void *rbuf, const void *wbuf);

/* "Management" callbacks */
static herr_t H5VL_dtio_init(hid_t vipl_id);
static herr_t H5VL_dtio_term(void);

/* VOL info callbacks */
static void  *H5VL_dtio_info_copy(const void *info);
static herr_t H5VL_dtio_info_cmp(int *cmp_value, const void *info1, const void *info2);
static herr_t H5VL_dtio_info_free(void *info);
static herr_t H5VL_dtio_info_to_str(const void *info, char **str);
static herr_t H5VL_dtio_str_to_info(const char *str, void **info);

static htri_t H5VL_dtio_need_bkg(hid_t src_type_id, hid_t dst_type_id,
    size_t *dst_type_size, hbool_t *fill_bkg);
static herr_t H5VL_dtio_tconv_init(hid_t src_type_id, size_t *src_type_size,
    hid_t dst_type_id, size_t *dst_type_size, hbool_t *_types_equal,
    H5VL_dtio_tconv_reuse_t *reuse, hbool_t *_need_bkg, hbool_t *fill_bkg);
static herr_t H5VL_dtio_get_selected_chunk_info(hid_t dcpl_id,
    hid_t file_space_id, hid_t mem_space_id,
    H5VL_dtio_select_chunk_info_t **chunk_info, size_t *chunk_info_len);
static herr_t H5VL_dtio_scatter_cb(const void **src_buf,
				   size_t *src_buf_bytes_used, void *_udata);

/* TODO temporary signatures Operations on dataspace selection iterators */
hid_t H5Ssel_iter_create(hid_t spaceid, size_t elmt_size, unsigned flags);
herr_t H5Ssel_iter_get_seq_list(hid_t sel_iter_id, size_t maxseq,
    size_t maxbytes, size_t *nseq, size_t *nbytes, hsize_t *off, size_t *len);
herr_t H5Ssel_iter_close(hid_t sel_iter_id);

/* VOL object wrap / retrieval callbacks */
// static void  *H5VL_dtio_get_object(const void *obj);
// static herr_t H5VL_dtio_get_wrap_ctx(const void *obj, void **wrap_ctx);
// static void  *H5VL_dtio_wrap_object(void *obj, H5I_type_t obj_type, void *wrap_ctx);
// static void  *H5VL_dtio_unwrap_object(void *obj);
// static herr_t H5VL_dtio_free_wrap_ctx(void *obj);

/* Attribute callbacks */
// static void  *H5VL_dtio_attr_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
//                                             hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id,
//                                             hid_t dxpl_id, void **req);
// static void  *H5VL_dtio_attr_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
//                                           hid_t aapl_id, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_attr_read(void *attr, hid_t mem_type_id, void *buf, hid_t dxpl_id,
//                                           void **req);
// static herr_t H5VL_dtio_attr_write(void *attr, hid_t mem_type_id, const void *buf, hid_t dxpl_id,
//                                            void **req);
// static herr_t H5VL_dtio_attr_get(void *obj, H5VL_attr_get_args_t *args, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_attr_specific(void *obj, const H5VL_loc_params_t *loc_params,
//                                               H5VL_attr_specific_args_t *args, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_attr_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id,
//                                               void **req);
// static herr_t H5VL_dtio_attr_close(void *attr, hid_t dxpl_id, void **req);

/* Dataset callbacks */
static void  *H5VL_dtio_dataset_create(void *obj, const H5VL_loc_params_t *loc_params,
                                               const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id,
                                               hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
static void  *H5VL_dtio_dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
                                             hid_t dapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_dtio_dataset_read(size_t count, void *dset[], hid_t mem_type_id[],
                                             hid_t mem_space_id[], hid_t file_space_id[], hid_t plist_id,
                                             void *buf[], void **req);
static herr_t H5VL_dtio_dataset_write(size_t count, void *dset[], hid_t mem_type_id[],
                                              hid_t mem_space_id[], hid_t file_space_id[], hid_t plist_id,
                                              const void *buf[], void **req);
static herr_t H5VL_dtio_dataset_get(void *dset, H5VL_dataset_get_args_t *args, hid_t dxpl_id,
                                            void **req);
// static herr_t H5VL_dtio_dataset_specific(void *obj, H5VL_dataset_specific_args_t *args, hid_t dxpl_id,
//                                                  void **req);
// static herr_t H5VL_dtio_dataset_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id,
//                                                  void **req);
static herr_t H5VL_dtio_dataset_close(void *dset, hid_t dxpl_id, void **req);

/* Datatype callbacks */
// static void *H5VL_dtio_datatype_commit(void *obj, const H5VL_loc_params_t *loc_params,
//                                                const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id,
//                                                hid_t tapl_id, hid_t dxpl_id, void **req);
// static void *H5VL_dtio_datatype_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
//                                              hid_t tapl_id, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_datatype_get(void *dt, H5VL_datatype_get_args_t *args, hid_t dxpl_id,
//                                              void **req);
// static herr_t H5VL_dtio_datatype_specific(void *obj, H5VL_datatype_specific_args_t *args,
//                                                   hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_datatype_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id,
//                                                   void **req);
// static herr_t H5VL_dtio_datatype_close(void *dt, hid_t dxpl_id, void **req);

/* File callbacks */
static void  *H5VL_dtio_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id,
                                            hid_t dxpl_id, void **req);
static void  *H5VL_dtio_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id,
                                          void **req);
// static herr_t H5VL_dtio_file_get(void *file, H5VL_file_get_args_t *args, hid_t dxpl_id, void **req);
static herr_t H5VL_dtio_file_specific(void *file, H5VL_file_specific_args_t *args, hid_t dxpl_id,
                                              void **req);
// static herr_t H5VL_dtio_file_optional(void *file, H5VL_optional_args_t *args, hid_t dxpl_id,
//                                               void **req);
static herr_t H5VL_dtio_file_close(void *file, hid_t dxpl_id, void **req);

/* Group callbacks */
// static void  *H5VL_dtio_group_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
//                                              hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id,
//                                              void **req);
// static void  *H5VL_dtio_group_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
//                                            hid_t gapl_id, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_group_get(void *obj, H5VL_group_get_args_t *args, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_group_specific(void *obj, H5VL_group_specific_args_t *args, hid_t dxpl_id,
//                                                void **req);
// static herr_t H5VL_dtio_group_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id,
//                                                void **req);
// static herr_t H5VL_dtio_group_close(void *grp, hid_t dxpl_id, void **req);

/* Link callbacks */
// static herr_t H5VL_dtio_link_create(H5VL_link_create_args_t *args, void *obj,
//                                             const H5VL_loc_params_t *loc_params, hid_t lcpl_id, hid_t lapl_id,
//                                             hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_link_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
//                                           const H5VL_loc_params_t *loc_params2, hid_t lcpl_id, hid_t lapl_id,
//                                           hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_link_move(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
//                                           const H5VL_loc_params_t *loc_params2, hid_t lcpl_id, hid_t lapl_id,
//                                           hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_link_get(void *obj, const H5VL_loc_params_t *loc_params,
//                                          H5VL_link_get_args_t *args, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_link_specific(void *obj, const H5VL_loc_params_t *loc_params,
//                                               H5VL_link_specific_args_t *args, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_link_optional(void *obj, const H5VL_loc_params_t *loc_params,
//                                               H5VL_optional_args_t *args, hid_t dxpl_id, void **req);

// /* Object callbacks */
// static void  *H5VL_dtio_object_open(void *obj, const H5VL_loc_params_t *loc_params,
//                                             H5I_type_t *opened_type, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_object_copy(void *src_obj, const H5VL_loc_params_t *src_loc_params,
//                                             const char *src_name, void *dst_obj,
//                                             const H5VL_loc_params_t *dst_loc_params, const char *dst_name,
//                                             hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_object_get(void *obj, const H5VL_loc_params_t *loc_params,
//                                            H5VL_object_get_args_t *args, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_object_specific(void *obj, const H5VL_loc_params_t *loc_params,
//                                                 H5VL_object_specific_args_t *args, hid_t dxpl_id, void **req);
// static herr_t H5VL_dtio_object_optional(void *obj, const H5VL_loc_params_t *loc_params,
//                                                 H5VL_optional_args_t *args, hid_t dxpl_id, void **req);

// /* Container/connector introspection callbacks */
static herr_t H5VL_dtio_introspect_get_conn_cls(void *obj, H5VL_get_conn_lvl_t lvl,
						const H5VL_class_t **conn_cls);
static herr_t H5VL_dtio_introspect_get_cap_flags(const void *info, uint64_t *cap_flags);
static herr_t H5VL_dtio_introspect_opt_query(void *obj, H5VL_subclass_t cls, int opt_type,
					     uint64_t *supported);
 
/* Async request callbacks */
// static herr_t H5VL_dtio_request_wait(void *req, uint64_t timeout, H5VL_request_status_t *status);
// static herr_t H5VL_dtio_request_notify(void *obj, H5VL_request_notify_t cb, void *ctx);
// static herr_t H5VL_dtio_request_cancel(void *req, H5VL_request_status_t *status);
// static herr_t H5VL_dtio_request_specific(void *req, H5VL_request_specific_args_t *args);
// static herr_t H5VL_dtio_request_optional(void *req, H5VL_optional_args_t *args);
// static herr_t H5VL_dtio_request_free(void *req);

/* Blob callbacks */
// static herr_t H5VL_dtio_blob_put(void *obj, const void *buf, size_t size, void *blob_id, void *ctx);
// static herr_t H5VL_dtio_blob_get(void *obj, const void *blob_id, void *buf, size_t size, void *ctx);
// static herr_t H5VL_dtio_blob_specific(void *obj, void *blob_id, H5VL_blob_specific_args_t *args);
// static herr_t H5VL_dtio_blob_optional(void *obj, void *blob_id, H5VL_optional_args_t *args);

/* Token callbacks */
// static herr_t H5VL_dtio_token_cmp(void *obj, const H5O_token_t *token1, const H5O_token_t *token2,
//                                           int *cmp_value);
// static herr_t H5VL_dtio_token_to_str(void *obj, H5I_type_t obj_type, const H5O_token_t *token,
//                                              char **token_str);
// static herr_t H5VL_dtio_token_from_str(void *obj, H5I_type_t obj_type, const char *token_str,
//                                                H5O_token_t *token);

/* Generic optional callback */
// static herr_t H5VL_dtio_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req);

/*******************/
/* Local variables */
/*******************/

/* Pass through VOL connector class struct */
static const H5VL_class_t H5VL_dtio_g = {
    H5VL_VERSION,                            /* VOL class struct version */
    (H5VL_class_value_t)DTIO_VOL_CONNECTOR_VALUE, /* value        */
    DTIO_VOL_CONNECTOR_NAME,                      /* name         */
    DTIO_VOL_CONNECTOR_VERSION,                   /* connector version */
    0,                                       /* capability flags */
    H5VL_dtio_init,                  /* initialize   */
    H5VL_dtio_term,                  /* terminate    */
    {
        /* info_cls */
        sizeof(H5VL_dtio_info_t), /* size    */
        H5VL_dtio_info_copy,      /* copy    */
        H5VL_dtio_info_cmp,       /* compare */
        H5VL_dtio_info_free,      /* free    */
        H5VL_dtio_info_to_str,    /* to_str  */
        H5VL_dtio_str_to_info     /* from_str */
    },
    {   /* wrap_cls */
        NULL,                                       /* get_object    */
        NULL,                                       /* get_wrap_ctx  */
        NULL,                                       /* wrap_object   */
        NULL,                                       /* unwrap_object */
        NULL                                        /* free_wrap_ctx */
    },
    // {
    //     /* wrap_cls */
    //     H5VL_dtio_get_object,    /* get_object   */
    //     H5VL_dtio_get_wrap_ctx,  /* get_wrap_ctx */
    //     H5VL_dtio_wrap_object,   /* wrap_object  */
    //     H5VL_dtio_unwrap_object, /* unwrap_object */
    //     H5VL_dtio_free_wrap_ctx  /* free_wrap_ctx */
    // }
    {   /* attribute_cls */
        NULL,                                       /* create */
        NULL,                                       /* open */
        NULL,                                       /* read */
        NULL,                                       /* write */
        NULL,                                       /* get */
        NULL,                                       /* specific */
        NULL,                                       /* optional */
        NULL,                                       /* close */
    },
    // {
    //     /* attribute_cls */
    //     H5VL_dtio_attr_create,   /* create */
    //     H5VL_dtio_attr_open,     /* open */
    //     H5VL_dtio_attr_read,     /* read */
    //     H5VL_dtio_attr_write,    /* write */
    //     H5VL_dtio_attr_get,      /* get */
    //     H5VL_dtio_attr_specific, /* specific */
    //     H5VL_dtio_attr_optional, /* optional */
    //     H5VL_dtio_attr_close     /* close */
    // }
    {
        /* dataset_cls */
        H5VL_dtio_dataset_create,   /* create */
        H5VL_dtio_dataset_open,     /* open */
        H5VL_dtio_dataset_read,     /* read */
        H5VL_dtio_dataset_write,    /* write */
        NULL, 	// H5VL_dtio_dataset_get,      /* get */
	NULL,         // H5VL_dtio_dataset_specific, /* specific */
	NULL,         // H5VL_dtio_dataset_optional, /* optional */
        H5VL_dtio_dataset_close     /* close */
    },
    {   /* datatype_cls */
        NULL,                                       /* commit */
        NULL,                                       /* open */
        NULL,                                       /* get */
        NULL,                                       /* specific */
        NULL,                                       /* optional */
        NULL,                                       /* close */
    },
    //     {
    //     /* datatype_cls */
    //     H5VL_dtio_datatype_commit,   /* commit */
    //     H5VL_dtio_datatype_open,     /* open */
    //     H5VL_dtio_datatype_get,      /* get_size */
    //     H5VL_dtio_datatype_specific, /* specific */
    //     H5VL_dtio_datatype_optional, /* optional */
    //     H5VL_dtio_datatype_close     /* close */
    // }
    {
        /* file_cls */
        H5VL_dtio_file_create,   /* create */
        H5VL_dtio_file_open,     /* open */
	NULL,
        // H5VL_dtio_file_get,      /* get */
	NULL,
        // H5VL_dtio_file_specific, /* specific */
	NULL,
        // H5VL_dtio_file_optional, /* optional */
        H5VL_dtio_file_close     /* close */
    },
    {
        /* group_cls */
        NULL,
        NULL,
	NULL,
	NULL,
	NULL,
	NULL
	// H5VL_dtio_group_create,   /* create */
	// H5VL_dtio_group_open,     /* open */
        // H5VL_dtio_group_get,      /* get */
        // H5VL_dtio_group_specific, /* specific */
        // H5VL_dtio_group_optional, /* optional */
        // H5VL_dtio_group_close     /* close */
    },
    {   /* link_cls */
        NULL,                                       /* create */
        NULL,                                       /* copy */
        NULL,                                       /* move */
        NULL,                                       /* get */
        NULL,                                       /* specific */
        NULL                                        /* optional */
    },
    {   /* object_cls */
        NULL,                                       /* open */
        NULL,                                       /* copy */
        NULL,                                       /* get */
        NULL,                                       /* specific */
        NULL,                                       /* optional */
    },
    {   /* introspect_cls */
      H5VL_dtio_introspect_get_conn_cls,            /* get_conn_cls  */
      H5VL_dtio_introspect_get_cap_flags,           /* get_cap_flags */
      H5VL_dtio_introspect_opt_query                /* opt_query     */
    },
    {   /* request_cls */
        NULL,                                       /* wait         */
        NULL,                                       /* notify       */
        NULL,                                       /* cancel       */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* free         */
    },
     {   /* blob_cls */
        NULL,                                       /* put          */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL                                        /* optional     */
    },
    {   /* token_cls */
        NULL,                                       /* cmp          */
        NULL,                                       /* to_str       */
        NULL                                        /* from_str     */
    },
    NULL                                            /* optional     */
    // {
    //     /* link_cls */
    //     H5VL_dtio_link_create,   /* create */
    //     H5VL_dtio_link_copy,     /* copy */
    //     H5VL_dtio_link_move,     /* move */
    //     H5VL_dtio_link_get,      /* get */
    //     H5VL_dtio_link_specific, /* specific */
    //     H5VL_dtio_link_optional  /* optional */
    // }
    // {
    //     /* object_cls */
    //     H5VL_dtio_object_open,     /* open */
    //     H5VL_dtio_object_copy,     /* copy */
    //     H5VL_dtio_object_get,      /* get */
    //     H5VL_dtio_object_specific, /* specific */
    //     H5VL_dtio_object_optional  /* optional */
    // },
    // {
    //     /* introspect_cls */
    //     H5VL_dtio_introspect_get_conn_cls,  /* get_conn_cls */
    //     H5VL_dtio_introspect_get_cap_flags, /* get_cap_flags */
    //     H5VL_dtio_introspect_opt_query,     /* opt_query */
    // },
    // {
    //     /* request_cls */
    //     H5VL_dtio_request_wait,     /* wait */
    //     H5VL_dtio_request_notify,   /* notify */
    //     H5VL_dtio_request_cancel,   /* cancel */
    //     H5VL_dtio_request_specific, /* specific */
    //     H5VL_dtio_request_optional, /* optional */
    //     H5VL_dtio_request_free      /* free */
    // },
    // {
    //     /* blob_cls */
    //     H5VL_dtio_blob_put,      /* put */
    //     H5VL_dtio_blob_get,      /* get */
    //     H5VL_dtio_blob_specific, /* specific */
    //     H5VL_dtio_blob_optional  /* optional */
    // },
    // {
    //     /* token_cls */
    //     H5VL_dtio_token_cmp,     /* cmp */
    //     H5VL_dtio_token_to_str,  /* to_str */
    //     H5VL_dtio_token_from_str /* from_str */
    // },
    // H5VL_dtio_optional /* optional */
};

/* The connector identification number, initialized at runtime */
static hid_t H5VL_DTIO_g = H5I_INVALID_HID;

/* Error stack declarations */
hid_t H5VL_ERR_STACK_g = H5I_INVALID_HID;
hid_t H5VL_ERR_CLS_g = H5I_INVALID_HID;

/*-------------------------------------------------------------------------
 * Function:    H5VL__dtio_new_obj
 *
 * Purpose:     Create a new pass through object for an underlying object
 *
 * Return:      Success:    Pointer to the new pass through object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static H5VL_dtio_t *
H5VL_dtio_new_obj(int fd, char *filename, char *dsetname, unsigned flags, hid_t space_id, hid_t type_id, hid_t dcpl_id)
{
    H5VL_dtio_t *new_obj;

    new_obj               = (H5VL_dtio_t *)calloc(1, sizeof(H5VL_dtio_t));
    new_obj->fd = fd;
    new_obj->file_name = filename;
    new_obj->file_name_len = strlen(filename);
    new_obj->dataset_name = dsetname;
    if (dsetname != NULL) {
      new_obj->dataset_name_len = strlen(dsetname);
    }
    else {
      new_obj->dataset_name_len = 0;
    }
    new_obj->flags = flags;
    new_obj->space_id = space_id;
    new_obj->type_id = type_id;
    new_obj->dcpl_id = dcpl_id;

    return new_obj;
} /* end H5VL__dtio_new_obj() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__dtio_free_obj
 *
 * Purpose:     Release a pass through object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_free_obj(H5VL_dtio_t *obj)
{
    hid_t err_id;

    err_id = H5Eget_current_stack();

    H5Eset_current_stack(err_id);

    free(obj);

    return 0;
} /* end H5VL__dtio_free_obj() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_register
 *
 * Purpose:     Register the pass-through VOL connector and retrieve an ID
 *              for it.
 *
 * Return:      Success:    The ID for the pass-through VOL connector
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5VL_dtio_register(void)
{
    /* Singleton register the pass-through VOL connector ID */
    if (H5VL_DTIO_g < 0)
        H5VL_DTIO_g = H5VLregister_connector(&H5VL_dtio_g, H5P_DEFAULT);

    return H5VL_DTIO_g;
} /* end H5VL_dtio_register() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_init
 *
 * Purpose:     Initialize this VOL connector, performing any necessary
 *              operations for the connector that will apply to all containers
 *              accessed with the connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_init(hid_t vipl_id)
{
#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL INIT\n");
#endif

    /* Shut compiler up about unused parameter */
    (void)vipl_id;
    dtio::hdf5::DTIO_Init();

    return 0;
} /* end H5VL_dtio_init() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_dtio_term
 *
 * Purpose:     Terminate this VOL connector, performing any necessary
 *              operations for the connector that release connector-wide
 *              resources (usually created / initialized with the 'init'
 *              callback).
 *
 * Return:      Success:    0
 *              Failure:    (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_term(void)
{
#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL TERM\n");
#endif

    /* Reset VOL ID */
    H5VL_DTIO_g = H5I_INVALID_HID;

    return 0;
} /* end H5VL_dtio_term() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_dtio_info_copy
 *
 * Purpose:     Duplicate the connector's info object.
 *
 * Returns:     Success:    New connector info object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
static void *
H5VL_dtio_info_copy(const void *_info)
{
    const H5VL_dtio_info_t *info = (const H5VL_dtio_info_t *)_info;
    H5VL_dtio_info_t       *new_info;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL INFO Copy\n");
#endif

    /* Make sure the underneath VOL of this pass-through VOL is specified */
    if (!info) {
        printf("\nH5VLdtio.c line %d in %s: info for pass-through VOL can't be null\n", __LINE__,
               __func__);
        return NULL;
    }

    /* Allocate new VOL info struct for the pass through connector */
    new_info = (H5VL_dtio_info_t *)calloc(1, sizeof(H5VL_dtio_info_t));

    strncpy(new_info->filename, info->filename, DTIO_HDF5_FILENAME_MAX);

    return new_info;
} /* end H5VL_dtio_info_copy() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_dtio_info_cmp
 *
 * Purpose:     Compare two of the connector's info objects, setting *cmp_value,
 *              following the same rules as strcmp().
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_info_cmp(int *cmp_value, const void *_info1, const void *_info2)
{
    const H5VL_dtio_info_t *info1 = (const H5VL_dtio_info_t *)_info1;
    const H5VL_dtio_info_t *info2 = (const H5VL_dtio_info_t *)_info2;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL INFO Compare\n");
#endif

    /* Sanity checks */
    assert(info1);
    assert(info2);

    /* Initialize comparison value */
    *cmp_value = strcmp(info1->filename, info2->filename);

    return 0;
} /* end H5VL_dtio_info_cmp() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_dtio_info_free
 *
 * Purpose:     Release an info object for the connector.
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_info_free(void *_info)
{
    H5VL_dtio_info_t *info = (H5VL_dtio_info_t *)_info;
    hid_t                     err_id;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL INFO Free\n");
#endif

    /* Free pass through info object itself */
    free(info);

    return 0;
} /* end H5VL_dtio_info_free() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_dtio_info_to_str
 *
 * Purpose:     Serialize an info object for this connector into a string
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_info_to_str(const void *_info, char **str)
{
    const H5VL_dtio_info_t *info              = (const H5VL_dtio_info_t *)_info;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL INFO To String\n");
#endif
    /* Allocate space for our info */
    size_t strSize = strlen(info->filename);
    *str           = (char *)H5allocate_memory(strSize, (bool)0);
    assert(*str);

    /* Encode our info */
    snprintf(*str, strSize, "%s", info->filename);

    return 0;
} /* end H5VL_dtio_info_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_dtio_str_to_info
 *
 * Purpose:     Deserialize a string into an info object for this connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_str_to_info(const char *str, void **_info)
{
    H5VL_dtio_info_t *info;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL INFO String To Info\n");
#endif
    /* Allocate new pass-through VOL connector info and set its fields */
    info                 = (H5VL_dtio_info_t *)calloc(1, sizeof(H5VL_dtio_info_t));
    strncpy(info->filename, str, DTIO_HDF5_FILENAME_MAX);

    /* Set return value */
    *_info = info;

    return 0;
} /* end H5VL_dtio_str_to_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_dtio_get_object
 *
 * Purpose:     Retrieve the 'data' for a VOL object.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
// static void *
// H5VL_dtio_get_object(const void *obj)
// {
//     const H5VL_dtio_t *o = (const H5VL_dtio_t *)obj;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL Get object\n");
// #endif

//     return H5VLget_object(o->under_object, o->under_vol_id);
// } /* end H5VL_dtio_get_object() */

// /*---------------------------------------------------------------------------
//  * Function:    H5VL_dtio_get_wrap_ctx
//  *
//  * Purpose:     Retrieve a "wrapper context" for an object
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *---------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_get_wrap_ctx(const void *obj, void **wrap_ctx)
// {
//     const H5VL_dtio_t    *o = (const H5VL_dtio_t *)obj;
//     H5VL_dtio_wrap_ctx_t *new_wrap_ctx;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL WRAP CTX Get\n");
// #endif

//     /* Allocate new VOL object wrapping context for the pass through connector */
//     new_wrap_ctx = (H5VL_dtio_wrap_ctx_t *)calloc(1, sizeof(H5VL_dtio_wrap_ctx_t));

//     /* Increment reference count on underlying VOL ID, and copy the VOL info */
//     new_wrap_ctx->under_vol_id = o->under_vol_id;

//     H5Iinc_ref(new_wrap_ctx->under_vol_id);

//     H5VLget_wrap_ctx(o->under_object, o->under_vol_id, &new_wrap_ctx->under_wrap_ctx);

//     /* Set wrap context to return */
//     *wrap_ctx = new_wrap_ctx;

//     return 0;
// } /* end H5VL_dtio_get_wrap_ctx() */

// /*---------------------------------------------------------------------------
//  * Function:    H5VL_dtio_wrap_object
//  *
//  * Purpose:     Use a "wrapper context" to wrap a data object
//  *
//  * Return:      Success:    Pointer to wrapped object
//  *              Failure:    NULL
//  *
//  *---------------------------------------------------------------------------
//  */
// static void *
// H5VL_dtio_wrap_object(void *obj, H5I_type_t obj_type, void *_wrap_ctx)
// {
//     H5VL_dtio_wrap_ctx_t *wrap_ctx = (H5VL_dtio_wrap_ctx_t *)_wrap_ctx;
//     H5VL_dtio_t          *new_obj;
//     void                         *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL WRAP Object\n");
// #endif

//     /* Wrap the object with the underlying VOL */
//     under = H5VLwrap_object(obj, obj_type, wrap_ctx->under_vol_id, wrap_ctx->under_wrap_ctx);
//     if (under)
//         new_obj = H5VL_dtio_new_obj(under, wrap_ctx->under_vol_id);
//     else
//         new_obj = NULL;

//     return new_obj;
// } /* end H5VL_dtio_wrap_object() */

// /*---------------------------------------------------------------------------
//  * Function:    H5VL_dtio_unwrap_object
//  *
//  * Purpose:     Unwrap a wrapped object, discarding the wrapper, but returning
//  *		underlying object.
//  *
//  * Return:      Success:    Pointer to unwrapped object
//  *              Failure:    NULL
//  *
//  *---------------------------------------------------------------------------
//  */
// static void *
// H5VL_dtio_unwrap_object(void *obj)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     void                *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL UNWRAP Object\n");
// #endif

//     /* Unrap the object with the underlying VOL */
//     under = H5VLunwrap_object(o->under_object, o->under_vol_id);

//     if (under)
//         H5VL_dtio_free_obj(o);

//     return under;
// } /* end H5VL_dtio_unwrap_object() */

// /*---------------------------------------------------------------------------
//  * Function:    H5VL_dtio_free_wrap_ctx
//  *
//  * Purpose:     Release a "wrapper context" for an object
//  *
//  * Note:	Take care to preserve the current HDF5 error stack
//  *		when calling HDF5 API calls.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *---------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_free_wrap_ctx(void *_wrap_ctx)
// {
//     H5VL_dtio_wrap_ctx_t *wrap_ctx = (H5VL_dtio_wrap_ctx_t *)_wrap_ctx;
//     hid_t                         err_id;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL WRAP CTX Free\n");
// #endif

//     err_id = H5Eget_current_stack();

//     /* Release underlying VOL ID and wrap context */
//     if (wrap_ctx->under_wrap_ctx)
//         H5VLfree_wrap_ctx(wrap_ctx->under_wrap_ctx, wrap_ctx->under_vol_id);
//     H5Idec_ref(wrap_ctx->under_vol_id);

//     H5Eset_current_stack(err_id);

//     /* Free pass through wrap context object itself */
//     free(wrap_ctx);

//     return 0;
// } /* end H5VL_dtio_free_wrap_ctx() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_attr_create
//  *
//  * Purpose:     Creates an attribute on an object.
//  *
//  * Return:      Success:    Pointer to attribute object
//  *              Failure:    NULL
//  *
//  *-------------------------------------------------------------------------
//  */
// static void *
// H5VL_dtio_attr_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t type_id,
//                               hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *attr;
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     void                *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL ATTRIBUTE Create\n");
// #endif

//     under = H5VLattr_create(o->under_object, loc_params, o->under_vol_id, name, type_id, space_id, acpl_id,
//                             aapl_id, dxpl_id, req);
//     if (under) {
//         attr = H5VL_dtio_new_obj(under, o->under_vol_id);

//         /* Check for async request */
//         if (req && *req)
//             *req = H5VL_dtio_new_obj(*req, o->under_vol_id);
//     } /* end if */
//     else
//         attr = NULL;

//     return (void *)attr;
// } /* end H5VL_dtio_attr_create() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_attr_open
//  *
//  * Purpose:     Opens an attribute on an object.
//  *
//  * Return:      Success:    Pointer to attribute object
//  *              Failure:    NULL
//  *
//  *-------------------------------------------------------------------------
//  */
// static void *
// H5VL_dtio_attr_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t aapl_id,
//                             hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *attr;
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     void                *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL ATTRIBUTE Open\n");
// #endif

//     under = H5VLattr_open(o->under_object, loc_params, o->under_vol_id, name, aapl_id, dxpl_id, req);
//     if (under) {
//         attr = H5VL_dtio_new_obj(under, o->under_vol_id);

//         /* Check for async request */
//         if (req && *req)
//             *req = H5VL_dtio_new_obj(*req, o->under_vol_id);
//     } /* end if */
//     else
//         attr = NULL;

//     return (void *)attr;
// } /* end H5VL_dtio_attr_open() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_attr_read
//  *
//  * Purpose:     Reads data from attribute.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_attr_read(void *attr, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)attr;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL ATTRIBUTE Read\n");
// #endif

//     ret_value = H5VLattr_read(o->under_object, o->under_vol_id, mem_type_id, buf, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_attr_read() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_attr_write
//  *
//  * Purpose:     Writes data to attribute.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_attr_write(void *attr, hid_t mem_type_id, const void *buf, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)attr;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL ATTRIBUTE Write\n");
// #endif

//     ret_value = H5VLattr_write(o->under_object, o->under_vol_id, mem_type_id, buf, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_attr_write() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_attr_get
//  *
//  * Purpose:     Gets information about an attribute
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_attr_get(void *obj, H5VL_attr_get_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL ATTRIBUTE Get\n");
// #endif

//     ret_value = H5VLattr_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_attr_get() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_attr_specific
//  *
//  * Purpose:     Specific operation on attribute
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_attr_specific(void *obj, const H5VL_loc_params_t *loc_params,
//                                 H5VL_attr_specific_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL ATTRIBUTE Specific\n");
// #endif

//     ret_value = H5VLattr_specific(o->under_object, loc_params, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_attr_specific() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_attr_optional
//  *
//  * Purpose:     Perform a connector-specific operation on an attribute
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_attr_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL ATTRIBUTE Optional\n");
// #endif

//     ret_value = H5VLattr_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_attr_optional() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_attr_close
//  *
//  * Purpose:     Closes an attribute.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1, attr not closed.
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_attr_close(void *attr, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)attr;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL ATTRIBUTE Close\n");
// #endif

//     ret_value = H5VLattr_close(o->under_object, o->under_vol_id, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     /* Release our wrapper, if underlying attribute was closed */
//     if (ret_value >= 0)
//         H5VL_dtio_free_obj(o);

//     return ret_value;
// } /* end H5VL_dtio_attr_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_dataset_create
 *
 * Purpose:     Creates a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_dtio_dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
                                 hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id,
                                 hid_t dxpl_id, void **req)
{
    H5VL_dtio_t *dset;
    H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
    // void                *under;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL DATASET Create\n");
#endif

    int result;
    char *dsetname = strdup(name);
    result = dtio::hdf5::DTIO_open(o->file_name, dsetname, o->flags, true, false);
    // under = H5VLdataset_create(o->under_object, loc_params, o->under_vol_id, name, lcpl_id, type_id, space_id,
    //                            dcpl_id, dapl_id, dxpl_id, req);
    if (result != -1) {
      dset = H5VL_dtio_new_obj(result, o->file_name, dsetname, o->flags, space_id, type_id, dcpl_id);
      /* Check for async request */
      // if (req && *req)
      // 	*req = H5VL_dtio_new_obj(name, o->fd, *req, result, space_id, type_id, dcpl_id);
    } /* end if */
    else
        dset = NULL;

    return (void *)dset;
} /* end H5VL_dtio_dataset_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_dataset_open
 *
 * Purpose:     Opens a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_dtio_dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
                               hid_t dapl_id, hid_t dxpl_id, void **req)
{
    H5VL_dtio_t *dset;
    H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
    void                *under;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL DATASET Open\n");
#endif

    // under = H5VLdataset_open(o->under_object, loc_params, o->under_vol_id, name, dapl_id, dxpl_id, req);
    int result;
    result = dtio::hdf5::DTIO_open(o->file_name, o->dataset_name, o->flags, true, false);

    if (result != -1) {
      dset = H5VL_dtio_new_obj(result, o->file_name, o->dataset_name, o->flags, o->space_id, o->type_id, o->dcpl_id);

      /* Check for async request */
      // if (req && *req)
      // 	*req = H5VL_dtio_new_obj(name, o->fd, *req, space_id, type_id, dcpl_id);
    } /* end if */
    else
        dset = NULL;

    return (void *)dset;
} /* end H5VL_dtio_dataset_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_dataset_read
 *
 * Purpose:     Reads data elements from a dataset into a buffer.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_dataset_read(size_t count, void *_dset[], hid_t mem_type_id[], hid_t mem_space_id[],
                               hid_t file_space_id[], hid_t plist_id, void *buf[], void **req)
{
  FUNC_ENTER_VOL(herr_t, SUCCEED)

    int dsetnum;
    H5VL_dtio_select_chunk_info_t *chunk_info = NULL; /* Array of info for each chunk selected in the file */
    H5VL_dtio_t *dset = NULL; // (H5VL_dtio_dset_t *)_dset;
    hid_t sel_iter_id; /* Selection iteration info */
    hbool_t sel_iter_init = false; /* Selection iteration info has been initialized */
    int ndims;
    hsize_t dim[H5S_MAX_RANK];
    hid_t real_file_space_id;
    hid_t real_mem_space_id;
    hssize_t num_elem;
    hssize_t num_elem_chunk;
    size_t chunk_info_len;
    char *chunk_oid = NULL;
    hbool_t read_op_init = false;
    size_t file_type_size = 0;
    size_t mem_type_size;
    hbool_t types_equal = true;
    hbool_t need_bkg = false;
    hbool_t fill_bkg = false;
    void *tmp_tconv_buf = NULL;
    void *tmp_bkg_buf = NULL;
    void *tconv_buf;
    void *bkg_buf;
    hbool_t close_spaces = false;
    H5VL_dtio_tconv_reuse_t reuse = H5VL_DTIO_TCONV_REUSE_NONE;
    int ret;
    uint64_t i;

    for (dsetnum = 0; dsetnum < count; dsetnum++) {
      dset = (H5VL_dtio_t *)_dset[dsetnum];

      /* Get dataspace extent */
      if((ndims = H5Sget_simple_extent_ndims(dset->space_id)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of dimensions");
      if(ndims != H5Sget_simple_extent_dims(dset->space_id, dim, NULL))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dimensions");

      /* Get "real" file space */
      if(file_space_id[dsetnum] == H5S_ALL)
        real_file_space_id = dset->space_id;
      else
        real_file_space_id = file_space_id[dsetnum];

      /* Get number of elements in selection */
      if((num_elem = H5Sget_select_npoints(real_file_space_id)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of points in selection");

      /* Get "real" file space */
      if(mem_space_id[dsetnum] == H5S_ALL)
        real_mem_space_id = real_file_space_id;
      else {
        hssize_t num_elem_file;

        real_mem_space_id = mem_space_id[dsetnum];

        /* Verify number of elements in memory selection matches file selection
         */
        if((num_elem_file = H5Sget_select_npoints(real_mem_space_id)) < 0)
	  HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of points in selection");
        if(num_elem_file != num_elem)
	  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "src and dest data spaces have different sizes");
      } /* end else */

      /* Check for no selection */
      if(num_elem == 0)
        HGOTO_DONE(SUCCEED);

      /* Initialize type conversion */
      if(H5VL_dtio_tconv_init(dset->type_id, &file_type_size, mem_type_id[dsetnum], &mem_type_size, &types_equal, &reuse, &need_bkg, &fill_bkg) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize type conversion");

      /* Check if the dataset actually has a chunked storage layout. If it does not, simply
       * set up the dataset as a single "chunk".
       */
      switch(H5Pget_layout(dset->dcpl_id)) {
      case H5D_COMPACT:
      case H5D_CONTIGUOUS:
	if (NULL == (chunk_info = (H5VL_dtio_select_chunk_info_t *)malloc(sizeof(H5VL_dtio_select_chunk_info_t))))
	  HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "can't allocate single chunk info buffer");
	chunk_info_len = 1;

	/* Set up "single-chunk dataset", with the "chunk" starting at coordinate 0 */
	chunk_info->fspace_id = real_file_space_id;
	chunk_info->mspace_id = real_mem_space_id;
	memset(chunk_info->chunk_coords, 0, sizeof(chunk_info->chunk_coords));

	break;

      case H5D_CHUNKED:
	/* Get the coordinates of the currently selected chunks in the file, setting up memory and file dataspaces for them */
	if(H5VL_dtio_get_selected_chunk_info(dset->dcpl_id, real_file_space_id, real_mem_space_id, &chunk_info, &chunk_info_len) < 0)
	  HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get selected chunk info");

	close_spaces = true;

	break;
      case H5D_LAYOUT_ERROR:
      case H5D_NLAYOUTS:
      case H5D_VIRTUAL:
      default:
	HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "invalid, unknown or unsupported dataset storage layout type");
      } /* end switch */

      /* Get number of elements in a chunk */
      if((num_elem_chunk = H5Sget_simple_extent_npoints(chunk_info[0].fspace_id)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of points in chunk");

      /* Iterate through each of the "chunks" in the dataset */
      for(i = 0; i < chunk_info_len; i++) {
        /* Create read op */
        // read_op = dtio_create_read_op();
        // read_op_init = true;

        /* Create chunk key */
        // if(H5VL_dtio_oid_create_chunk(dset->obj.item.file, dset->obj.bin_oid, ndims,
	// 			       chunk_info[i].chunk_coords, &chunk_oid) < 0)
	//   HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't create dataset chunk oid");

        /* Get number of elements in selection */
        if((num_elem = H5Sget_select_npoints(chunk_info[i].mspace_id)) < 0)
	  HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of points in selection");

        /* There was a former if block here... */
        {
	  htri_t match_select = false;

	  /* Check if the types are equal */
	  if(types_equal) {
	    /* No type conversion necessary */
	    /* Check if we should match the file and memory sequence lists
	     * (serialized selections).  We can do this if the memory space
	     * is H5S_ALL and the chunk extent equals the file extent.  If
	     * the number of chunks selected is more than one we do not need
	     * to check the extents because they cannot be the same.  We
	     * could also allow the case where the memory space is not
	     * H5S_ALL but is equivalent. */
	    if(mem_space_id[dsetnum] == H5S_ALL && chunk_info_len == 1)
	      if((match_select = H5Sextent_equal(real_file_space_id, chunk_info[i].fspace_id)) < 0)
		HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOMPARE, FAIL, "can't check if file and chunk dataspaces are equal");

	    /* Check for matching selections */
	    if(match_select) {
	      /* Build read op from file space */
	      if(H5VL_dtio_build_io_op_match(dset->file_name, dset->dataset_name, chunk_info[i].fspace_id, file_type_size, (size_t)num_elem, buf[dsetnum], NULL) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't generate DTIO read op");
	    } /* end if */
	    else {
	      /* Build read op from file space and mem space */
	      if(H5VL_dtio_build_io_op_merge(dset->file_name, dset->dataset_name, chunk_info[i].mspace_id, chunk_info[i].fspace_id, file_type_size, (size_t)num_elem, buf[dsetnum], NULL) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't generate DTIO read op");
	    } /* end else */

	    /* Read data from dataset */
	    // if((ret = dtio_read_op_operate(read_op, H5VL_dtio_params_g.dtio_ioctx, chunk_oid, LIBDTIO_OPERATION_NOFLAG)) < 0)
	    //   HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data from dataset: %s", strerror(-ret));
	  } /* end if */
	  else {
	    size_t nseq_tmp;
	    size_t nelem_tmp;
	    hsize_t sel_off;
	    size_t sel_len;
	    hbool_t contig;

	    /* Type conversion necessary */

	    /* Check for contiguous memory buffer */

	    /* Initialize selection iterator  */
	    if((sel_iter_id = H5Ssel_iter_create(chunk_info[i].mspace_id, (size_t)1, 0)) < 0)
	      HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
	    sel_iter_init = true;       /* Selection iteration info has been initialized */

	    /* Get the sequence list - only check the first sequence because we only
	     * care if it is contiguous and if so where the contiguous selection
	     * begins */
	    if(H5Ssel_iter_get_seq_list(sel_iter_id, (size_t)1, (size_t)-1, &nseq_tmp, &nelem_tmp, &sel_off, &sel_len) < 0)
	      HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "sequence length generation failed");
	    contig = (sel_len == (size_t)num_elem);
	    sel_off *= (hsize_t)mem_type_size;

	    /* Release selection iterator */
	    if(H5Ssel_iter_close(sel_iter_id) < 0)
	      HGOTO_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
	    sel_iter_init = false;

	    /* Find or allocate usable type conversion buffer */
	    if(contig && (reuse == H5VL_DTIO_TCONV_REUSE_TCONV))
	      tconv_buf = (char *)buf + (size_t)sel_off;
	    else {
	      if(!tmp_tconv_buf)
		if(NULL == (tmp_tconv_buf = malloc(
						   (size_t)num_elem_chunk * (file_type_size
									     > mem_type_size ? file_type_size
									     : mem_type_size))))
		  HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't allocate type conversion buffer");
	      tconv_buf = tmp_tconv_buf;
	    } /* end else */

	    /* Find or allocate usable background buffer */
	    if(need_bkg) {
	      if(contig && (reuse == H5VL_DTIO_TCONV_REUSE_BKG))
		bkg_buf = (char *)buf + (size_t)sel_off;
	      else {
		if(!tmp_bkg_buf)
		  if(NULL == (tmp_bkg_buf = malloc(
						   (size_t)num_elem_chunk * mem_type_size)))
		    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't allocate background buffer");
		bkg_buf = tmp_bkg_buf;
	      } /* end else */
	    } /* end if */
	    else
	      bkg_buf = NULL;

	    /* Build read op from file space */
	    if(H5VL_dtio_build_io_op_contig(dset->file_name, dset->dataset_name, chunk_info[i].fspace_id, file_type_size, (size_t)num_elem, tconv_buf, NULL) < 0)
	      HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't generate DTIO write op");

	    /* Read data from dataset */
	    // if((ret = dtio_read_op_operate(read_op, H5VL_dtio_params_g.dtio_ioctx, chunk_oid, LIBDTIO_OPERATION_NOFLAG)) < 0)
	    //   HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data from dataset: %s", strerror(-ret));

	    /* Gather data to background buffer if necessary */
	    if(fill_bkg && (bkg_buf == tmp_bkg_buf))
	      if(H5Dgather(chunk_info[i].mspace_id, buf[dsetnum], mem_type_id[dsetnum], (size_t)num_elem * mem_type_size, bkg_buf, NULL, NULL) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't gather data to background buffer");

	    /* Perform type conversion */
	    if(H5Tconvert(dset->type_id, mem_type_id[dsetnum], (size_t)num_elem, tconv_buf, bkg_buf, plist_id) < 0)
	      HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't perform type conversion");

	    /* Scatter data to memory buffer if necessary */
	    if(tconv_buf == tmp_tconv_buf) {
	      H5VL_dtio_scatter_cb_ud_t scatter_cb_ud;

	      scatter_cb_ud.buf = tconv_buf;
	      scatter_cb_ud.len = (size_t)num_elem * mem_type_size;
	      if(H5Dscatter(H5VL_dtio_scatter_cb, &scatter_cb_ud, mem_type_id[dsetnum], chunk_info[i].mspace_id, buf) < 0)
		HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't scatter data to read buffer");
	    } /* end if */
	  } /* end else */
        } /* end else */

        // dtio_release_read_op(read_op);
        // read_op_init = false;
      } /* end for */
    }
done:
    /* Free memory */
    // if(read_op_init)
    //     dtio_release_read_op(read_op);
    // free(chunk_oid);
    free(tmp_tconv_buf);
    free(tmp_bkg_buf);

    if(chunk_info) {
        if(close_spaces) {
            for(i = 0; i < chunk_info_len; i++) {
                if(H5Sclose(chunk_info[i].mspace_id) < 0)
                    HDONE_ERROR(H5E_DATASPACE, H5E_CANTCLOSEOBJ, FAIL, "can't close memory space");
                if(H5Sclose(chunk_info[i].fspace_id) < 0)
                    HDONE_ERROR(H5E_DATASPACE, H5E_CANTCLOSEOBJ, FAIL, "can't close file space");
            } /* end for */
        } /* end if */

        free(chunk_info);
    } /* end if */

    /* Release selection iterator */
    if(sel_iter_init && H5Ssel_iter_close(sel_iter_id) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");

    FUNC_LEAVE_VOL
} /* end H5VL_dtio_dataset_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_dataset_write
 *
 * Purpose:     Writes data elements from a buffer into a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_dataset_write(size_t count, void *_dset[], hid_t mem_type_id[], hid_t mem_space_id[],
                                hid_t file_space_id[], hid_t plist_id, const void *buf[], void **req)
{
  // void  *obj_local;        /* Local buffer for obj */
  // void **obj = &obj_local; /* Array of object pointers */
  // size_t i;                /* Local index variable */
  // herr_t ret_value;

  FUNC_ENTER_VOL(herr_t, SUCCEED)

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL DATASET Write\n");
// #endif

//     /* Allocate obj array if necessary */
//     if (count > 1)
//       if (NULL == (obj = (void **)malloc(count * sizeof(void *))))
// 	return -1;

//     /* Build obj array */
//     for (i = 0; i < count; i++) {
//         /* Get the object */
//         obj[i] = ((H5VL_dtio_t *)dset[i])->fd;

//         /* Make sure the class matches */
//         if (((H5VL_dtio_t *)dset[i])->fd != ((H5VL_dtio_t *)dset[0])->fd)
//             return -1;
//     }

//     // FIXME this is going to be an extremely complicated call, try to simplify it but if you can't then try to figure it out from the old Dtio VOL
//     ret_value = dtio::HDF5::DTIO_write(fd, buf, ??);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, ((H5VL_dtio_t *)dset[0])->under_vol_id);

//     /* Free memory */
//     if (obj != &obj_local)
//         free(obj);

//     return ret_value;

  int dsetnum;
  H5VL_dtio_select_chunk_info_t *chunk_info = NULL; /* Array of info for each chunk selected in the file */
  H5VL_dtio_t *dset = NULL;
  int ndims;
  hsize_t dim[H5S_MAX_RANK];
  hid_t real_file_space_id;
  hid_t real_mem_space_id;
  hssize_t num_elem;
  hssize_t num_elem_chunk;
  size_t chunk_info_len;
  task write_op;
  hbool_t write_op_init = false;
  task read_op;
  hbool_t read_op_init = false;
  size_t file_type_size;
  size_t mem_type_size;
  hbool_t types_equal = true;
  hbool_t need_bkg = false;
  hbool_t fill_bkg = false;
  void *tconv_buf = NULL;
  void *bkg_buf = NULL;
  hbool_t close_spaces = false;
  H5VL_dtio_tconv_reuse_t reuse = H5VL_DTIO_TCONV_REUSE_NONE;
  int ret;
  uint64_t i;
  for (dsetnum = 0; dsetnum < count; dsetnum++) {
    dset = (H5VL_dtio_t *)_dset[dsetnum];

    /* Check for write access */
    if(!(dset->flags & H5F_ACC_RDWR))
      HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "no write intent on file");

    /* Get dataspace extent */
    if((ndims = H5Sget_simple_extent_ndims(dset->space_id)) < 0)
      HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of dimensions");
    if(ndims != H5Sget_simple_extent_dims(dset->space_id, dim, NULL))
      HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dimensions");

    /* Get "real" file space */
    if(file_space_id[dsetnum] == H5S_ALL)
        real_file_space_id = dset->space_id;
    else
        real_file_space_id = file_space_id[dsetnum];

    /* Get number of elements in selection */
    if((num_elem = H5Sget_select_npoints(real_file_space_id)) < 0)
      HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of points in selection");

    /* Get "real" mem space */
    if(mem_space_id[dsetnum] == H5S_ALL)
        real_mem_space_id = real_file_space_id;
    else {
        hssize_t num_elem_file;

        real_mem_space_id = mem_space_id[dsetnum];

        /* Verify number of elements in memory selection matches file selection
         */
        if((num_elem_file = H5Sget_select_npoints(real_mem_space_id)) < 0)
	  HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of points in selection");
        if(num_elem_file != num_elem)
	  HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "src and dest data spaces have different sizes");
    } /* end else */

    /* Check for no selection */
    if(num_elem == 0)
      HGOTO_DONE(SUCCEED);

    /* Initialize type conversion */
    if(H5VL_dtio_tconv_init(dset->type_id, &file_type_size, mem_type_id[dsetnum], &mem_type_size, &types_equal, &reuse, &need_bkg, &fill_bkg) < 0)
      HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize type conversion");

    /* Check if the dataset actually has a chunked storage layout. If it does not, simply
     * set up the dataset as a single "chunk".
     */
    switch(H5Pget_layout(dset->dcpl_id)) {
        case H5D_COMPACT:
        case H5D_CONTIGUOUS:
            if (NULL == (chunk_info = (H5VL_dtio_select_chunk_info_t *)malloc(sizeof(H5VL_dtio_select_chunk_info_t))))
	      HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "can't allocate single chunk info buffer");
            chunk_info_len = 1;

            /* Set up "single-chunk dataset", with the "chunk" starting at coordinate 0 */
            chunk_info->fspace_id = real_file_space_id;
            chunk_info->mspace_id = real_mem_space_id;
            memset(chunk_info->chunk_coords, 0, sizeof(chunk_info->chunk_coords));

            break;

        case H5D_CHUNKED:
            /* Get the coordinates of the currently selected chunks in the file, setting up memory and file dataspaces for them */
            if(H5VL_dtio_get_selected_chunk_info(dset->dcpl_id, real_file_space_id, real_mem_space_id, &chunk_info, &chunk_info_len) < 0)
	      HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get selected chunk info");

            close_spaces = true;

            break;
        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        case H5D_VIRTUAL:
        default:
	  HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "invalid, unknown or unsupported dataset storage layout type");
    } /* end switch */

    /* Get number of elements in a chunk */
    if((num_elem_chunk = H5Sget_simple_extent_npoints(chunk_info[0].fspace_id)) < 0)
      HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of points in chunk");

    /* Allocate tconv_buf if necessary */
    if(!types_equal)
        if(NULL == (tconv_buf = malloc( (size_t)num_elem_chunk
                * (file_type_size > mem_type_size ? file_type_size
                : mem_type_size))))
	  HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't allocate type conversion buffer");

    /* Allocate bkg_buf if necessary */
    if(need_bkg)
        if(NULL == (bkg_buf = malloc((size_t)num_elem_chunk
                * mem_type_size)))
	  HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't allocate background buffer");

    /* Iterate through each of the "chunks" in the dataset */
    for(i = 0; i < chunk_info_len; i++) {
        /* Create write op */
        // write_op = dtio_create_write_op();
        // write_op_init = true;

        /* Create chunk key */
      // if(H5VL_dtio_create_chunkname(dset->file_name, dset->file_name_len, ndims,
      // 				    chunk_info[i].chunk_coords, &chunk_name) < 0)
      // 	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't create dataset chunk name");

        /* Get number of elements in selection */
        if((num_elem = H5Sget_select_npoints(chunk_info[i].mspace_id)) < 0)
	  HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of points in selection");

        /* Former if block here... */
        {
            htri_t match_select = false;

            /* Check if the types are equal */
            if(types_equal) {
                /* No type conversion necessary */
                /* Check if we should match the file and memory sequence lists
                 * (serialized selections).  We can do this if the memory space
                 * is H5S_ALL and the chunk extent equals the file extent.  If
                 * the number of chunks selected is more than one we do not need
                 * to check the extents because they cannot be the same.  We
                 * could also allow the case where the memory space is not
                 * H5S_ALL but is equivalent. */
                if(mem_space_id[dsetnum] == H5S_ALL && chunk_info_len == 1)
                    if((match_select = H5Sextent_equal(real_file_space_id, chunk_info[i].fspace_id)) < 0)
		      HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOMPARE, FAIL, "can't check if file and chunk dataspaces are equal");

                /* Check for matching selections */
                if(match_select) {
                    /* Build write op from file space */
		  // NULL, write_op
		  if(H5VL_dtio_build_io_op_match(dset->file_name, dset->dataset_name, chunk_info[i].fspace_id, file_type_size, (size_t)num_elem, NULL, buf[dsetnum]) < 0)
		      HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't generate DTIO write op");
                } /* end if */
                else {
                    /* Build write op from file space and mem space */
		  // NULL, write_op
		  if(H5VL_dtio_build_io_op_merge(dset->file_name, dset->dataset_name, chunk_info[i].mspace_id, chunk_info[i].fspace_id, file_type_size, (size_t)num_elem, NULL, buf[dsetnum]) < 0)
		      HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't generate DTIO write op");
                } /* end else */
            } /* end if */
            else {
                /* Type conversion necessary */
                /* Check if we need to fill background buffer */
                if(fill_bkg) {
                    assert(bkg_buf);

                    /* Create read op */
                    // read_op = dtio_create_read_op();
                    // read_op_init = true;

                    /* Build io ops (to read to bkg_buf and write from tconv_buf)
                     * from file space */
		    // read_op, write_op
                    if(H5VL_dtio_build_io_op_contig(dset->file_name, dset->dataset_name, chunk_info[i].fspace_id, file_type_size, (size_t)num_elem, bkg_buf, tconv_buf) < 0)
		      HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't generate DTIO write op");

                    /* Read data from dataset to background buffer */
                    // if((ret = dtio_read_op_operate(read_op, chunk_oid, LIBDTIO_OPERATION_NOFLAG)) < 0)
		    //   HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data from dataset: %s", strerror(-ret));

                    // dtio_release_read_op(read_op);
                    // read_op_init = false;
                } /* end if */
                else {
		  /* Build write op from file space */
		  // NULL, write_op
		  if(H5VL_dtio_build_io_op_contig(dset->file_name, dset->dataset_name, chunk_info[i].fspace_id, file_type_size, (size_t)num_elem, NULL, tconv_buf) < 0)
		    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't generate DTIO write op");
		}

                /* Gather data to conversion buffer */
                if(H5Dgather(chunk_info[i].mspace_id, buf[dsetnum], mem_type_id[dsetnum], (size_t)num_elem * mem_type_size, tconv_buf, NULL, NULL) < 0)
		  HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't gather data to conversion buffer");

                /* Perform type conversion */
                if(H5Tconvert(mem_type_id[dsetnum], dset->type_id, (size_t)num_elem, tconv_buf, bkg_buf, plist_id) < 0)
		  HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "can't perform type conversion");
            } /* end else */

            /* Write data to dataset */
            // if((ret = dtio_write_op_operate(write_op, chunk_oid, NULL, LIBDTIO_OPERATION_NOFLAG)) < 0)
	    //   HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "can't write data to dataset: %s", strerror(-ret));
        } /* end else */

        // dtio_release_write_op(write_op);
        write_op_init = false;
    } /* end for */
  }

 done:
      /* Free memory */
    // if(read_op_init)
    //     dtio_release_read_op(read_op);
    // if(write_op_init)
    //     dtio_release_write_op(write_op);
    // free(chunk_name);
    free(tconv_buf);
    free(bkg_buf);

    if(chunk_info) {
        if(close_spaces) {
            for(i = 0; i < chunk_info_len; i++) {
                if(H5Sclose(chunk_info[i].mspace_id) < 0)
                    HDONE_ERROR(H5E_DATASPACE, H5E_CANTCLOSEOBJ, FAIL, "can't close memory space");
                if(H5Sclose(chunk_info[i].fspace_id) < 0)
                    HDONE_ERROR(H5E_DATASPACE, H5E_CANTCLOSEOBJ, FAIL, "can't close file space");
            } /* end for */
        } /* end if */

        free(chunk_info);
    } /* end if */

    FUNC_LEAVE_VOL
} /* end H5VL_dtio_dataset_write() */

static herr_t
H5VL_dtio_build_io_op_match(char *filename, char *dsetname, hid_t file_space_id, size_t type_size,
    size_t tot_nelem, void *rbuf, const void *wbuf)
{
    hid_t sel_iter_id;              /* Selection iteration info */
    hbool_t sel_iter_init = false;  /* Selection iteration info has been initialized */
    size_t nseq;
    size_t nelem;
    hsize_t off[H5VL_DTIO_SEQ_LIST_LEN];
    size_t len[H5VL_DTIO_SEQ_LIST_LEN];
    size_t szi;

    FUNC_ENTER_VOL(herr_t, SUCCEED)

    if (!rbuf == !wbuf)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to use passed buffers");
    assert(!rbuf != !wbuf);
    assert(tot_nelem > 0);

    /* Initialize selection iterator  */
    if((sel_iter_id = H5Ssel_iter_create(file_space_id, type_size, 0)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    sel_iter_init = true;       /* Selection iteration info has been initialized */

    /* Generate sequences from the file space until finished */
    do {
        /* Get the sequences of bytes */
        if(H5Ssel_iter_get_seq_list(sel_iter_id, (size_t)H5VL_DTIO_SEQ_LIST_LEN, (size_t)-1, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "sequence length generation failed");
        tot_nelem -= nelem;

        /* Create io ops from offsets and lengths */
        if(rbuf) {
	  for(szi = 0; szi < nseq; szi++) {
	    dtio::hdf5::DTIO_read(filename, dsetname, (char *)rbuf + off[szi], len[szi], (uint64_t)off[szi]);
	  }
	}
	else {
	  for(szi = 0; szi < nseq; szi++) {
	    dtio::hdf5::DTIO_write(filename, dsetname, (const char *)wbuf + off[szi], len[szi], (uint64_t)off[szi]);
	  }
	}
    } while(tot_nelem > 0);

done:
    /* Release selection iterator */
    if(sel_iter_init && H5Ssel_iter_close(sel_iter_id) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");

    FUNC_LEAVE_VOL
}

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_need_bkg
 *
 * Purpose:     Determine if a background buffer is needed for conversion.
 *
 * Return:      Success:        0
 *              Failure:        -1
 *
 * Programmer:  Neil Fortner
 *              March, 2018
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5VL_dtio_need_bkg(hid_t src_type_id, hid_t dst_type_id, size_t *dst_type_size,
    hbool_t *fill_bkg)
{
    hid_t memb_type_id = -1;
    hid_t src_memb_type_id = -1;
    char *memb_name = NULL;
    size_t memb_size;
    H5T_class_t tclass;

    FUNC_ENTER_VOL(htri_t, FALSE)

    /* Get destination type size */
    if((*dst_type_size = H5Tget_size(dst_type_id)) == 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get source type size");

    /* Get datatype class */
    if(H5T_NO_CLASS == (tclass = H5Tget_class(dst_type_id)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get type class");

    switch(tclass) {
        case H5T_INTEGER:
        case H5T_FLOAT:
        case H5T_TIME:
        case H5T_STRING:
        case H5T_BITFIELD:
        case H5T_OPAQUE:
        case H5T_ENUM:
            /* No background buffer necessary */
            break;

        case H5T_COMPOUND:
            {
                int nmemb;
                size_t size_used = 0;
                int src_i;
                int i;

                /* We must always provide a background buffer for compound
                 * conversions.  Only need to check further to see if it must be
                 * filled. */
                FUNC_RETURN_SET(TRUE);

                /* Get number of compound members */
                if((nmemb = H5Tget_nmembers(dst_type_id)) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get number of destination compound members");

                /* Iterate over compound members, checking for a member in
                 * dst_type_id with no match in src_type_id */
                for(i = 0; i < nmemb; i++) {
                    /* Get member type */
                    if((memb_type_id = H5Tget_member_type(dst_type_id, (unsigned)i)) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get compound member type");

                    /* Get member name */
                    if(NULL == (memb_name = H5Tget_member_name(dst_type_id, (unsigned)i)))
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get compound member name");

                    /* Check for matching name in source type */
                    H5E_BEGIN_TRY {
                        src_i = H5Tget_member_index(src_type_id, memb_name);
                    } H5E_END_TRY

                    /* Free memb_name */
                    if(H5free_memory(memb_name) < 0)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTFREE, FAIL, "can't free member name");
                    memb_name = NULL;

                    /* If no match was found, this type is not being filled in,
                     * so we must fill the background buffer */
                    if(src_i < 0) {
                        if(H5Tclose(memb_type_id) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "can't close member type");
                        memb_type_id = -1;
                        *fill_bkg = TRUE;
                        HGOTO_DONE(TRUE);
                    } /* end if */

                    /* Open matching source type */
                    if((src_memb_type_id = H5Tget_member_type(src_type_id, (unsigned)src_i)) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get compound member type");

                    /* Recursively check member type, this will fill in the
                     * member size */
                    if(H5VL_dtio_need_bkg(src_memb_type_id, memb_type_id, &memb_size, fill_bkg) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "can't check if background buffer needed");

                    /* Close source member type */
                    if(H5Tclose(src_memb_type_id) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "can't close member type");
                    src_memb_type_id = -1;

                    /* Close member type */
                    if(H5Tclose(memb_type_id) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "can't close member type");
                    memb_type_id = -1;

                    /* If the source member type needs the background filled, so
                     * does the parent */
                    if(*fill_bkg)
                        HGOTO_DONE(TRUE);

                    /* Keep track of the size used in compound */
                    size_used += memb_size;
                } /* end for */

                /* Check if all the space in the type is used.  If not, we must
                 * fill the background buffer. */
                /* TODO: This is only necessary on read, we don't care about
                 * compound gaps in the "file" DSMINC */
                assert(size_used <= *dst_type_size);
                if(size_used != *dst_type_size)
                    *fill_bkg = TRUE;

                break;
            } /* end block */

        case H5T_ARRAY:
            /* Get parent type */
            if((memb_type_id = H5Tget_super(dst_type_id)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get array parent type");

            /* Get source parent type */
            if((src_memb_type_id = H5Tget_super(src_type_id)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get array parent type");

            /* Recursively check parent type */
            if((FUNC_RETURN_SET(H5VL_dtio_need_bkg(src_memb_type_id, memb_type_id, &memb_size, fill_bkg))) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "can't check if background buffer needed");

            /* Close source parent type */
            if(H5Tclose(src_memb_type_id) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "can't close array parent type");
            src_memb_type_id = -1;

            /* Close parent type */
            if(H5Tclose(memb_type_id) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "can't close array parent type");
            memb_type_id = -1;

            break;

        case H5T_REFERENCE:
        case H5T_VLEN:
            /* Not yet supported */
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "reference and vlen types not supported");

            break;

        case H5T_NO_CLASS:
        case H5T_NCLASSES:
        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, FAIL, "invalid type class");
    } /* end switch */

done:
    /* Cleanup on failure */
    if(FUNC_ERRORED) {
        if(memb_type_id >= 0)
            if(H5Idec_ref(memb_type_id) < 0)
                HDONE_ERROR(H5E_DATATYPE, H5E_CANTDEC, FAIL, "failed to close member type");
        if(src_memb_type_id >= 0)
            if(H5Idec_ref(src_memb_type_id) < 0)
                HDONE_ERROR(H5E_DATATYPE, H5E_CANTDEC, FAIL, "failed to close source member type");
        free(memb_name);
    } /* end if */

    FUNC_LEAVE_VOL
} /* end H5VL_dtio_need_bkg() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_tconv_init
 *
 * Purpose:     DSMINC
 *
 * Return:      Success:        0
 *              Failure:        -1
 *
 * Programmer:  Neil Fortner
 *              April, 2018
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_tconv_init(hid_t src_type_id, size_t *src_type_size,
    hid_t dst_type_id, size_t *dst_type_size, hbool_t *_types_equal,
    H5VL_dtio_tconv_reuse_t *reuse, hbool_t *_need_bkg, hbool_t *fill_bkg)
{
    htri_t need_bkg = FALSE;
    htri_t types_equal;

    FUNC_ENTER_VOL(herr_t, SUCCEED)

    assert(src_type_size);
    assert(dst_type_size);
    assert(_types_equal);
    assert(_need_bkg);
    assert(fill_bkg);
    assert(!*fill_bkg);

    /* Get source type size */
    if((*src_type_size = H5Tget_size(src_type_id)) == 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get source type size");

    /* Check if the types are equal */
    if((types_equal = H5Tequal(src_type_id, dst_type_id)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOMPARE, FAIL, "can't check if types are equal");
    if(types_equal)
        /* Types are equal, no need for conversion, just set dst_type_size */
        *dst_type_size = *src_type_size;
    else {
        /* Check if we need a background buffer */
        if((need_bkg = H5VL_dtio_need_bkg(src_type_id, dst_type_id, dst_type_size, fill_bkg)) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "can't check if background buffer needed");

        /* Check for reusable destination buffer */
        if(reuse) {
            assert(*reuse == H5VL_DTIO_TCONV_REUSE_NONE);

            /* Use dest buffer for type conversion if it large enough, otherwise
             * use it for the background buffer if one is needed. */
            if(dst_type_size >= src_type_size)
                *reuse = H5VL_DTIO_TCONV_REUSE_TCONV;
            else if(need_bkg)
                *reuse = H5VL_DTIO_TCONV_REUSE_BKG;
        } /* end if */
    } /* end else */

    /* Set return values */
    *_types_equal = types_equal;
    *_need_bkg = need_bkg;

done:
    /* Cleanup on failure */
    if(FUNC_ERRORED) {
        *reuse = H5VL_DTIO_TCONV_REUSE_NONE;
    } /* end if */

    FUNC_LEAVE_VOL
} /* end H5VL_dtio_tconv_init() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_get_selected_chunk_info
 *
 * Purpose:     Calculates the starting coordinates for the chunks selected
 *              in the file space given by file_space_id and sets up
 *              individual memory and file spaces for each chunk. The chunk
 *              coordinates and dataspaces are returned through the
 *              chunk_info struct pointer.
 *
 *              XXX: Note that performance could be increased by
 *                   calculating all of the chunks in the entire dataset
 *                   and then caching them in the dataset object for
 *                   re-use in subsequent reads/writes
 *
 * Return:      Success: 0
 *              Failure: -1
 *
 * Programmer:  Neil Fortner
 *              May, 2018
 *              Based on H5VL_daosm_get_selected_chunk_info by Jordan
 *              Henderson, May, 2017
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_get_selected_chunk_info(hid_t dcpl,
    hid_t file_space_id, hid_t mem_space_id,
    H5VL_dtio_select_chunk_info_t **chunk_info, size_t *chunk_info_len)
{
    H5VL_dtio_select_chunk_info_t *_chunk_info = NULL;
    hssize_t  num_sel_points;
    hssize_t  chunk_file_space_adjust[H5O_LAYOUT_NDIMS];
    hsize_t   chunk_dims[H5S_MAX_RANK];
    hsize_t   file_sel_start[H5S_MAX_RANK], file_sel_end[H5S_MAX_RANK];
    hsize_t   mem_sel_start[H5S_MAX_RANK], mem_sel_end[H5S_MAX_RANK];
    hsize_t   start_coords[H5O_LAYOUT_NDIMS], end_coords[H5O_LAYOUT_NDIMS];
    hsize_t   selection_start_coords[H5O_LAYOUT_NDIMS] = {0};
    hsize_t   num_sel_points_cast;
    htri_t    space_same_shape = FALSE;
    size_t    info_buf_alloced;
    size_t    i, j;
    int       fspace_ndims, mspace_ndims;
    int       increment_dim;

    FUNC_ENTER_VOL(herr_t, SUCCEED)

    assert(chunk_info);
    assert(chunk_info_len);

    if ((num_sel_points = H5Sget_select_npoints(file_space_id)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADVALUE, FAIL, "can't get number of points select in dataspace");
//    H5_CHECKED_ASSIGN(num_sel_points_cast, hsize_t, num_sel_points, hssize_t);
    num_sel_points_cast = (hsize_t) num_sel_points;

    /* Get the chunking information */
    if (H5Pget_chunk(dcpl, H5S_MAX_RANK, chunk_dims) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get chunking information");

    if ((fspace_ndims = H5Sget_simple_extent_ndims(file_space_id)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get file space dimensionality");
    if ((mspace_ndims = H5Sget_simple_extent_ndims(mem_space_id)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get memory space dimensionality");
    assert(mspace_ndims == fspace_ndims);

    /* Get the bounding box for the current selection in the file space */
    if (H5Sget_select_bounds(file_space_id, file_sel_start, file_sel_end) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get bounding box for file selection");

    if (H5Sget_select_bounds(mem_space_id, mem_sel_start, mem_sel_end) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get bounding box for memory selection");

    /* Calculate the adjustment for memory selection from the file selection */
    for (i = 0; i < (size_t) fspace_ndims; i++) {
//        H5_CHECK_OVERFLOW(file_sel_start[i], hsize_t, hssize_t);
//        H5_CHECK_OVERFLOW(mem_sel_start[i], hsize_t, hssize_t);
        chunk_file_space_adjust[i] = (hssize_t) file_sel_start[i] - (hssize_t) mem_sel_start[i];
    } /* end for */

    if (NULL == (_chunk_info = (H5VL_dtio_select_chunk_info_t *) malloc(H5VL_DTIO_DEFAULT_NUM_SEL_CHUNKS * sizeof(*_chunk_info))))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "can't allocate space for selected chunk info buffer");
    info_buf_alloced = H5VL_DTIO_DEFAULT_NUM_SEL_CHUNKS * sizeof(*_chunk_info);

    /* Calculate the coordinates for the initial chunk */
    for (i = 0; i < (size_t) fspace_ndims; i++) {
        start_coords[i] = selection_start_coords[i] = (file_sel_start[i] / chunk_dims[i]) * chunk_dims[i];
        end_coords[i] = (start_coords[i] + chunk_dims[i]) - 1;
    } /* end for */

    if (FAIL == (space_same_shape = H5Sselect_shape_same(file_space_id, mem_space_id)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADVALUE, FAIL, "not a dataspace");

    /* Iterate through each "chunk" in the dataset */
    for (i = 0; num_sel_points_cast;) {
        htri_t intersect = FALSE;

        if((intersect = H5Sselect_intersect_block(file_space_id, start_coords, end_coords)) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADVALUE, FAIL, "cannot determine intersection");
        /* Check for intersection of file selection and "chunk". If there is
         * an intersection, set up a valid memory and file space for the chunk. */
        if (TRUE == intersect) {
            hssize_t  chunk_mem_space_adjust[H5O_LAYOUT_NDIMS];
            hssize_t  chunk_sel_npoints;
            hid_t     tmp_chunk_fspace_id;

            /* Re-allocate selected chunk info buffer if necessary */
            while (i > (info_buf_alloced / sizeof(*_chunk_info)) - 1) {
                if (NULL == (_chunk_info = (H5VL_dtio_select_chunk_info_t *) realloc(_chunk_info, 2 * info_buf_alloced)))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "can't reallocate space for selected chunk info buffer");
                info_buf_alloced *= 2;
            } /* end while */

            /*
             * Set up the file Dataspace for this chunk.
             */

            /* Create temporary chunk for selection operations */
            if ((tmp_chunk_fspace_id = H5Scopy(file_space_id)) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy file space");

            /* Make certain selections are stored in span tree form (not "optimized hyperslab" or "all") */
            // TODO check whether this is still necessary after hyperslab update merge
//            if (H5Shyper_convert(tmp_chunk_fspace_id) < 0) {
//                H5Sclose(tmp_chunk_fspace_id);
//                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to convert selection to span trees");
//            } /* end if */

            /* "AND" temporary chunk and current chunk */
            if (H5Sselect_hyperslab(tmp_chunk_fspace_id, H5S_SELECT_AND, start_coords, NULL, chunk_dims, NULL) < 0) {
                H5Sclose(tmp_chunk_fspace_id);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't create chunk selection");
            } /* end if */

            /* Resize chunk's dataspace dimensions to size of chunk */
            if (H5Sset_extent_simple(tmp_chunk_fspace_id, fspace_ndims, chunk_dims, NULL) < 0) {
                H5Sclose(tmp_chunk_fspace_id);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't adjust chunk dimensions");
            } /* end if */

	    /* Move selection back to have correct offset in chunk */
	    hssize_t sstart_coords [H5O_LAYOUT_NDIMS]; // FIXME this is dirty, just to get it to compile, probably have to think of something more elegant in the long run
	    for (i = 0; i < H5O_LAYOUT_NDIMS; i++) {
	      sstart_coords[i] = (hssize_t) start_coords[i];
	    }
            /* Move selection back to have correct offset in chunk */
            if (H5Sselect_adjust(tmp_chunk_fspace_id, sstart_coords) < 0) {
                H5Sclose(tmp_chunk_fspace_id);
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't adjust chunk selection");
            } /* end if */

            /* Copy the chunk's coordinates to the selected chunk info buffer */
            memcpy(_chunk_info[i].chunk_coords, start_coords, (size_t) fspace_ndims * sizeof(hsize_t));

            _chunk_info[i].fspace_id = tmp_chunk_fspace_id;

            /*
             * Now set up the memory Dataspace for this chunk.
             */
            if (space_same_shape) {
                hid_t  tmp_chunk_mspace_id;

                if ((tmp_chunk_mspace_id = H5Scopy(mem_space_id)) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy memory space");

                /* Release the current selection */
                // if (H5Sselect_release(tmp_chunk_mspace_id) < 0) {
                //     H5Sclose(tmp_chunk_mspace_id);
                //     HGOTO_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection");
                // } /* end if */

                /* Copy the chunk's file space selection to its memory space selection */
                if (H5Sselect_copy(tmp_chunk_mspace_id, tmp_chunk_fspace_id) < 0) {
                    H5Sclose(tmp_chunk_mspace_id);
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "unable to copy selection");
                } /* end if */

                /* Compute the adjustment for the chunk */
                for (j = 0; j < (size_t) fspace_ndims; j++) {
//                    H5_CHECK_OVERFLOW(_chunk_info[i].chunk_coords[j], hsize_t, hssize_t);
                    chunk_mem_space_adjust[j] = chunk_file_space_adjust[j] - (hssize_t) _chunk_info[i].chunk_coords[j];
                } /* end for */

                /* Adjust the selection */
                if (H5Sselect_adjust(tmp_chunk_mspace_id, chunk_mem_space_adjust) < 0) {
                    H5Sclose(tmp_chunk_mspace_id);
                    HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSELECT, FAIL, "can't adjust chunk memory space selection");
                } /* end if */

                _chunk_info[i].mspace_id = tmp_chunk_mspace_id;
            } /* end if */
            else {
                HGOTO_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL, "file and memory selections must currently have the same shape");
            } /* end else */

            i++;

            /* Determine if there are more chunks to process */
            if ((chunk_sel_npoints = H5Sget_select_npoints(tmp_chunk_fspace_id)) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't get number of points selected in chunk file space");

            num_sel_points_cast -= (hsize_t) chunk_sel_npoints;

            if (num_sel_points_cast == 0)
                HGOTO_DONE(SUCCEED);
        } /* end if */

        /* Set current increment dimension */
        increment_dim = fspace_ndims - 1;

        /* Increment chunk location in fastest changing dimension */
//        H5_CHECK_OVERFLOW(chunk_dims[increment_dim], hsize_t, hssize_t);
        start_coords[increment_dim] += chunk_dims[increment_dim];
        end_coords[increment_dim] += chunk_dims[increment_dim];

        /* Bring chunk location back into bounds, if necessary */
        if (start_coords[increment_dim] > file_sel_end[increment_dim]) {
            do {
                /* Reset current dimension's location to 0 */
                start_coords[increment_dim] = selection_start_coords[increment_dim];
                end_coords[increment_dim] = (start_coords[increment_dim] + chunk_dims[increment_dim]) - 1;

                /* Decrement current dimension */
                increment_dim--;

                /* Increment chunk location in current dimension */
                start_coords[increment_dim] += chunk_dims[increment_dim];
                end_coords[increment_dim] = (start_coords[increment_dim] + chunk_dims[increment_dim]) - 1;
            } while (start_coords[increment_dim] > file_sel_end[increment_dim]);
        } /* end if */
    } /* end for */

done:
    if (FUNC_ERRORED) {
        if (_chunk_info)
            free(_chunk_info);
    } else {
        *chunk_info = _chunk_info;
        *chunk_info_len = i;
    } /* end else */

    FUNC_LEAVE_VOL
} /* end H5VL_dtio_get_selected_chunk_info() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_scatter_cb
 *
 * Purpose:     Callback function for H5Dscatter.  Simply passes the
 *              entire buffer described by udata to H5Dscatter.
 *
 * Return:      SUCCEED (never fails)
 *
 * Programmer:  Neil Fortner
 *              April, 2018
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_scatter_cb(const void **src_buf, size_t *src_buf_bytes_used,
    void *_udata)
{
    H5VL_dtio_scatter_cb_ud_t *udata = (H5VL_dtio_scatter_cb_ud_t *)_udata;

    FUNC_ENTER_VOL(herr_t, SUCCEED)

    /* Set src_buf and src_buf_bytes_used to use the entire buffer */
    *src_buf = udata->buf;
    *src_buf_bytes_used = udata->len;

    FUNC_LEAVE_VOL
} /* end H5VL_dtio_scatter_cb() */

static herr_t
H5VL_dtio_build_io_op_merge(char *filename, char *dsetname, hid_t mem_space_id, hid_t file_space_id,
    size_t type_size, size_t tot_nelem, void *rbuf, const void *wbuf)
{
    hid_t mem_sel_iter_id;              /* Selection iteration info */
    hbool_t mem_sel_iter_init = false;  /* Selection iteration info has been initialized */
    hid_t file_sel_iter_id;             /* Selection iteration info */
    hbool_t file_sel_iter_init = false; /* Selection iteration info has been initialized */
    size_t mem_nseq = 0;
    size_t file_nseq = 0;
    size_t nelem;
    hsize_t mem_off[H5VL_DTIO_SEQ_LIST_LEN];
    size_t mem_len[H5VL_DTIO_SEQ_LIST_LEN];
    hsize_t file_off[H5VL_DTIO_SEQ_LIST_LEN];
    size_t file_len[H5VL_DTIO_SEQ_LIST_LEN];
    size_t io_len;
    size_t tot_len = tot_nelem * type_size;
    size_t mem_i = 0;
    size_t file_i = 0;
    size_t mem_ei = 0;
    size_t file_ei = 0;

    FUNC_ENTER_VOL(herr_t, SUCCEED)

    assert(!rbuf != !wbuf);
    assert(tot_nelem > 0);

    /* Initialize selection iterators  */
    if((mem_sel_iter_id = H5Ssel_iter_create(mem_space_id, type_size, 0)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    mem_sel_iter_init = true;       /* Selection iteration info has been initialized */
    if((file_sel_iter_id = H5Ssel_iter_create(file_space_id, type_size, 0)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    file_sel_iter_init = true;       /* Selection iteration info has been initialized */

    /* Generate sequences from the file space until finished */
    do {
        /* Get the sequences of bytes if necessary */
        assert(mem_i <= mem_nseq);
        if(mem_i == mem_nseq) {
            if(H5Ssel_iter_get_seq_list(mem_sel_iter_id, (size_t)H5VL_DTIO_SEQ_LIST_LEN, (size_t)-1, &mem_nseq, &nelem, mem_off, mem_len) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "sequence length generation failed");
            mem_i = 0;
        } /* end if */
        assert(file_i <= file_nseq);
        if(file_i == file_nseq) {
            if(H5Ssel_iter_get_seq_list(file_sel_iter_id, (size_t)H5VL_DTIO_SEQ_LIST_LEN, (size_t)-1, &file_nseq, &nelem, file_off, file_len) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "sequence length generation failed");
            file_i = 0;
        } /* end if */

        /* Calculate number of elements to put in next merged offset/length
         * pair */
        io_len = mem_len[mem_i] <= file_len[file_i] ? mem_len[mem_i] : file_len[file_i];

        /* Add to I/O op */
        if(rbuf) {
	  dtio::hdf5::DTIO_read(filename, dsetname, (char *)rbuf + mem_off[mem_i] + mem_ei, io_len, (uint64_t)(file_off[file_i] + file_ei));
	}
	else {
	  dtio::hdf5::DTIO_write(filename, dsetname, (const char *)wbuf + mem_off[mem_i] + mem_ei, io_len, (uint64_t)(file_off[file_i] + file_ei));
	}

        /* Update indices */
        if(io_len == mem_len[mem_i]) {
            mem_i++;
            mem_ei = 0;
        } /* end if */
        else {
            assert(mem_len[mem_i] > io_len);
            mem_len[mem_i] -= io_len;
            mem_ei += io_len;
        } /* end else */
        if(io_len == file_len[file_i]) {
            file_i++;
            file_ei = 0;
        } /* end if */
        else {
            assert(file_len[file_i] > io_len);
            file_len[file_i] -= io_len;
            file_ei += io_len;
        } /* end else */
        tot_len -= io_len;
    } while(tot_len > 0);

done:
    /* Release selection iterators */
    if(mem_sel_iter_init && H5Ssel_iter_close(mem_sel_iter_id) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");
    if(file_sel_iter_init && H5Ssel_iter_close(file_sel_iter_id) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");

    FUNC_LEAVE_VOL
}

static herr_t
H5VL_dtio_build_io_op_contig(char *filename, char *dsetname, hid_t file_space_id, size_t type_size,
    size_t tot_nelem, void *rbuf, const void *wbuf)
{
    hid_t sel_iter_id;              /* Selection iteration info */
    hbool_t sel_iter_init = false;  /* Selection iteration info has been initialized */
    size_t nseq;
    size_t nelem;
    hsize_t off[H5VL_DTIO_SEQ_LIST_LEN];
    size_t len[H5VL_DTIO_SEQ_LIST_LEN];
    size_t mem_off = 0;
    size_t szi;

    FUNC_ENTER_VOL(herr_t, SUCCEED)

    assert(rbuf || wbuf);
    assert(tot_nelem > 0);

    /* Initialize selection iterator  */
    if((sel_iter_id = H5Ssel_iter_create(file_space_id, type_size, 0)) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTINIT, FAIL, "unable to initialize selection iterator");
    sel_iter_init = true;       /* Selection iteration info has been initialized */

    /* Generate sequences from the file space until finished */
    do {
        /* Get the sequences of bytes */
        if(H5Ssel_iter_get_seq_list(sel_iter_id, (size_t)H5VL_DTIO_SEQ_LIST_LEN, (size_t)-1, &nseq, &nelem, off, len) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "sequence length generation failed");
        tot_nelem -= nelem;

        /* Create io ops from offsets and lengths */
        for(szi = 0; szi < nseq; szi++) {
	  if(rbuf) {
	    dtio::hdf5::DTIO_read(filename, dsetname, (char *)rbuf + mem_off, len[szi], (uint64_t)off[szi]);
	  }
	  if(wbuf) {
	    dtio::hdf5::DTIO_write(filename, dsetname, (const char *)wbuf + mem_off, len[szi], (uint64_t)off[szi]);
	  }
            mem_off += len[szi];
        } /* end for */
    } while(tot_nelem > 0);

done:
    /* Release selection iterator */
    if(sel_iter_init && H5Ssel_iter_close(sel_iter_id) < 0)
        HDONE_ERROR(H5E_DATASPACE, H5E_CANTRELEASE, FAIL, "unable to release selection iterator");

    FUNC_LEAVE_VOL
}

// Probably, update layout in dtio_t, adjust dtio vol dataset_write to
// be applicable again. The biggest concern that I have is the
// complication of the dataset_write, especially a later part that
// constructs and executes the write operation. I'll probably have to
// figure out more as I go, but I'm pretty sure about using the existing
// layout to discover the structure of the I/O call.


/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_dataset_get
 *
 * Purpose:     Gets information about a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_dataset_get(void *dset, H5VL_dataset_get_args_t *args, hid_t dxpl_id, void **req)
{
    H5VL_dtio_t *o = (H5VL_dtio_t *)dset;
    herr_t               ret_value;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL DATASET Get\n");
#endif

    // ret_value = H5VLdataset_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

    /* Check for async request */
    // if (req && *req)
    //     *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

    return ret_value;
} /* end H5VL_dtio_dataset_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_dataset_specific
 *
 * Purpose:     Specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
// static herr_t
// H5VL_dtio_dataset_specific(void *obj, H5VL_dataset_specific_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     hid_t                under_vol_id;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL H5Dspecific\n");
// #endif

//     /* Save copy of underlying VOL connector ID, in case of
//      * 'refresh' operation destroying the current object
//      */
//     under_vol_id = o->under_vol_id;

//     ret_value = H5VLdataset_specific(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_dataset_specific() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_dataset_optional
//  *
//  * Purpose:     Perform a connector-specific operation on a dataset
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_dataset_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL DATASET Optional\n");
// #endif

//     ret_value = H5VLdataset_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_dataset_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_dataset_close
 *
 * Purpose:     Closes a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1, dataset not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_dataset_close(void *dset, hid_t dxpl_id, void **req)
{
    H5VL_dtio_t *o = (H5VL_dtio_t *)dset;
    herr_t               ret_value;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL DATASET Close\n");
#endif

    // ret_value = H5VLdataset_close(o->under_object, o->under_vol_id, dxpl_id, req);

    dtio::hdf5::DTIO_close(o->fd);
    /* Check for async request */
    // if (req && *req)
    //     *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

    /* Release our wrapper, if underlying dataset was closed */
    if (ret_value >= 0)
        H5VL_dtio_free_obj(o);

    return ret_value;
} /* end H5VL_dtio_dataset_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_datatype_commit
 *
 * Purpose:     Commits a datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
// static void *
// H5VL_dtio_datatype_commit(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
//                                   hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t dxpl_id,
//                                   void **req)
// {
//     H5VL_dtio_t *dt;
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     void                *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL DATATYPE Commit\n");
// #endif

//     under = H5VLdatatype_commit(o->under_object, loc_params, o->under_vol_id, name, type_id, lcpl_id, tcpl_id,
//                                 tapl_id, dxpl_id, req);
//     if (under) {
//         dt = H5VL_dtio_new_obj(under, o->under_vol_id);

//         /* Check for async request */
//         if (req && *req)
//             *req = H5VL_dtio_new_obj(*req, o->under_vol_id);
//     } /* end if */
//     else
//         dt = NULL;

//     return (void *)dt;
// } /* end H5VL_dtio_datatype_commit() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_datatype_open
//  *
//  * Purpose:     Opens a named datatype inside a container.
//  *
//  * Return:      Success:    Pointer to datatype object
//  *              Failure:    NULL
//  *
//  *-------------------------------------------------------------------------
//  */
// static void *
// H5VL_dtio_datatype_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
//                                 hid_t tapl_id, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *dt;
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     void                *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL DATATYPE Open\n");
// #endif

//     under = H5VLdatatype_open(o->under_object, loc_params, o->under_vol_id, name, tapl_id, dxpl_id, req);
//     if (under) {
//         dt = H5VL_dtio_new_obj(under, o->under_vol_id);

//         /* Check for async request */
//         if (req && *req)
//             *req = H5VL_dtio_new_obj(*req, o->under_vol_id);
//     } /* end if */
//     else
//         dt = NULL;

//     return (void *)dt;
// } /* end H5VL_dtio_datatype_open() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_datatype_get
//  *
//  * Purpose:     Get information about a datatype
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_datatype_get(void *dt, H5VL_datatype_get_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)dt;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL DATATYPE Get\n");
// #endif

//     ret_value = H5VLdatatype_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_datatype_get() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_datatype_specific
//  *
//  * Purpose:     Specific operations for datatypes
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_datatype_specific(void *obj, H5VL_datatype_specific_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     hid_t                under_vol_id;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL DATATYPE Specific\n");
// #endif

//     /* Save copy of underlying VOL connector ID, in case of
//      * 'refresh' operation destroying the current object
//      */
//     under_vol_id = o->under_vol_id;

//     ret_value = H5VLdatatype_specific(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_datatype_specific() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_datatype_optional
//  *
//  * Purpose:     Perform a connector-specific operation on a datatype
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_datatype_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL DATATYPE Optional\n");
// #endif

//     ret_value = H5VLdatatype_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_datatype_optional() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_datatype_close
//  *
//  * Purpose:     Closes a datatype.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1, datatype not closed.
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_datatype_close(void *dt, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)dt;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL DATATYPE Close\n");
// #endif

//     assert(o->under_object);

//     ret_value = H5VLdatatype_close(o->under_object, o->under_vol_id, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     /* Release our wrapper, if underlying datatype was closed */
//     if (ret_value >= 0)
//         H5VL_dtio_free_obj(o);

//     return ret_value;
// } /* end H5VL_dtio_datatype_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_file_create
 *
 * Purpose:     Creates a container using this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_dtio_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id,
                              void **req)
{
    H5VL_dtio_info_t *info;
    H5VL_dtio_t      *file;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL FILE Create\n");
#endif

    /* Get copy of our VOL info from FAPL */
    H5Pget_vol_info(fapl_id, (void **)&info);

    /* Make sure we have info about the underlying VOL to be used */
    if (!info)
        return NULL;

    /* Open the file with the underlying VOL connector */
    char *filename = strdup(name);
    int fd;
    fd = dtio::hdf5::DTIO_open(filename, NULL, flags, true, true);
    if (fd != -1) {
      file = H5VL_dtio_new_obj(fd, filename, NULL, flags, -1, -1, -1);

      /* Check for async request */
      // if (req && *req)
      // 	*req = H5VL_dtio_new_obj(name, *req, -1, flags, -1, -1, -1);
    } /* end if */
    else
      file = NULL;

    /* Release copy of our VOL info */
    H5VL_dtio_info_free(info);

    return (void *)file;
} /* end H5VL_dtio_file_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_file_open
 *
 * Purpose:     Opens a container created with this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_dtio_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req)
{
    H5VL_dtio_info_t *info;
    H5VL_dtio_t      *file;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL FILE Open\n");
#endif

    /* Get copy of our VOL info from FAPL */
    H5Pget_vol_info(fapl_id, (void **)&info);

    /* Make sure we have info about the underlying VOL to be used */
    if (!info)
        return NULL;

    /* Open the file with the underlying VOL connector */
    int fd;
    char *filename = strdup(name);
    fd = dtio::hdf5::DTIO_open(name, NULL, flags, false, true);
    if (fd != -1) {
      file = H5VL_dtio_new_obj(fd, filename, NULL, flags, -1, -1, -1);

      /* Check for async request */
      // if (req && *req)
      // 	*req = H5VL_dtio_new_obj(name, *req, -1, flags, -1, -1, -1);
    } /* end if */
    else
        file = NULL;

    /* Close underlying FAPL */
    // H5Pclose(under_fapl_id);

    /* Release copy of our VOL info */
    H5VL_dtio_info_free(info);

    return (void *)file;
} /* end H5VL_dtio_file_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_file_get
 *
 * Purpose:     Get info about a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
// static herr_t
// H5VL_dtio_file_get(void *file, H5VL_file_get_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)file;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL FILE Get\n");
// #endif

//     ret_value = H5VLfile_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_file_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_file_specific
 *
 * Purpose:     Specific operation on file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
// static herr_t
// H5VL_dtio_file_specific(void *file, H5VL_file_specific_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t       *o = (H5VL_dtio_t *)file;
//     H5VL_dtio_t       *new_o;
//     H5VL_file_specific_args_t  my_args;
//     H5VL_file_specific_args_t *new_args;
//     H5VL_dtio_info_t  *info         = NULL;
//     hid_t                      under_vol_id = -1;
//     herr_t                     ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL FILE Specific\n");
// #endif

//     if (args->op_type == H5VL_FILE_IS_ACCESSIBLE) {
//         /* Shallow copy the args */
//         memcpy(&my_args, args, sizeof(my_args));

//         /* Get copy of our VOL info from FAPL */
//         H5Pget_vol_info(args->args.is_accessible.fapl_id, (void **)&info);

//         /* Make sure we have info about the underlying VOL to be used */
//         if (!info)
//             return (-1);

//         /* Keep the correct underlying VOL ID for later */
//         under_vol_id = info->under_vol_id;

//         /* Copy the FAPL */
//         my_args.args.is_accessible.fapl_id = H5Pcopy(args->args.is_accessible.fapl_id);

//         /* Set the VOL ID and info for the underlying FAPL */
//         H5Pset_vol(my_args.args.is_accessible.fapl_id, info->under_vol_id, info->under_vol_info);

//         /* Set argument pointer to new arguments */
//         new_args = &my_args;

//         /* Set object pointer for operation */
//         new_o = NULL;
//     } /* end else-if */
//     else if (args->op_type == H5VL_FILE_DELETE) {
//         /* Shallow copy the args */
//         memcpy(&my_args, args, sizeof(my_args));

//         /* Get copy of our VOL info from FAPL */
//         H5Pget_vol_info(args->args.del.fapl_id, (void **)&info);

//         /* Make sure we have info about the underlying VOL to be used */
//         if (!info)
//             return (-1);

//         /* Keep the correct underlying VOL ID for later */
//         under_vol_id = info->under_vol_id;

//         /* Copy the FAPL */
//         my_args.args.del.fapl_id = H5Pcopy(args->args.del.fapl_id);

//         /* Set the VOL ID and info for the underlying FAPL */
//         H5Pset_vol(my_args.args.del.fapl_id, info->under_vol_id, info->under_vol_info);

//         /* Set argument pointer to new arguments */
//         new_args = &my_args;

//         /* Set object pointer for operation */
//         new_o = NULL;
//     } /* end else-if */
//     else {
//         /* Keep the correct underlying VOL ID for later */
//         under_vol_id = o->under_vol_id;

//         /* Set argument pointer to current arguments */
//         new_args = args;

//         /* Set object pointer for operation */
//         new_o = o->under_object;
//     } /* end else */

//     ret_value = H5VLfile_specific(new_o, under_vol_id, new_args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, under_vol_id);

//     if (args->op_type == H5VL_FILE_IS_ACCESSIBLE) {
//         /* Close underlying FAPL */
//         H5Pclose(my_args.args.is_accessible.fapl_id);

//         /* Release copy of our VOL info */
//         H5VL_dtio_info_free(info);
//     } /* end else-if */
//     else if (args->op_type == H5VL_FILE_DELETE) {
//         /* Close underlying FAPL */
//         H5Pclose(my_args.args.del.fapl_id);

//         /* Release copy of our VOL info */
//         H5VL_dtio_info_free(info);
//     } /* end else-if */
//     else if (args->op_type == H5VL_FILE_REOPEN) {
//         /* Wrap file struct pointer for 'reopen' operation, if we reopened one */
//         if (ret_value >= 0 && *args->args.reopen.file)
//             *args->args.reopen.file = H5VL_dtio_new_obj(*args->args.reopen.file, under_vol_id);
//     } /* end else */

//     return ret_value;
// } /* end H5VL_dtio_file_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_file_optional
 *
 * Purpose:     Perform a connector-specific operation on a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
// static herr_t
// H5VL_dtio_file_optional(void *file, H5VL_optional_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)file;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL File Optional\n");
// #endif

//     ret_value = H5VLfile_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_file_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_file_close
 *
 * Purpose:     Closes a file.
 *
 * Return:      Success:    0
 *              Failure:    -1, file not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_dtio_file_close(void *file, hid_t dxpl_id, void **req)
{
    H5VL_dtio_t *o = (H5VL_dtio_t *)file;
    herr_t               ret_value;

#ifdef ENABLE_DTIO_LOGGING
    printf("------- DTIO VOL FILE Close\n");
#endif

    ret_value = dtio::hdf5::DTIO_close(o->fd);

    /* Check for async request */
    // if (req && *req)
    //   *req = H5VL_dtio_new_obj(*req, -1, flags, -1, -1, -1);

    /* Release our wrapper, if underlying file was closed */
    if (ret_value >= 0)
        H5VL_dtio_free_obj(o);

    return ret_value;
} /* end H5VL_dtio_file_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_group_create
 *
 * Purpose:     Creates a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
// static void *
// H5VL_dtio_group_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
//                                hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *group;
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     void                *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL GROUP Create\n");
// #endif

//     under = H5VLgroup_create(o->under_object, loc_params, o->under_vol_id, name, lcpl_id, gcpl_id, gapl_id,
//                              dxpl_id, req);
//     if (under) {
//         group = H5VL_dtio_new_obj(under, o->under_vol_id);

//         /* Check for async request */
//         if (req && *req)
//             *req = H5VL_dtio_new_obj(*req, o->under_vol_id);
//     } /* end if */
//     else
//         group = NULL;

//     return (void *)group;
// } /* end H5VL_dtio_group_create() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_group_open
//  *
//  * Purpose:     Opens a group inside a container
//  *
//  * Return:      Success:    Pointer to a group object
//  *              Failure:    NULL
//  *
//  *-------------------------------------------------------------------------
//  */
// static void *
// H5VL_dtio_group_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t gapl_id,
//                              hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *group;
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     void                *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL GROUP Open\n");
// #endif

//     under = H5VLgroup_open(o->under_object, loc_params, o->under_vol_id, name, gapl_id, dxpl_id, req);
//     if (under) {
//         group = H5VL_dtio_new_obj(under, o->under_vol_id);

//         /* Check for async request */
//         if (req && *req)
//             *req = H5VL_dtio_new_obj(*req, o->under_vol_id);
//     } /* end if */
//     else
//         group = NULL;

//     return (void *)group;
// } /* end H5VL_dtio_group_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_group_get
 *
 * Purpose:     Get info about a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
// static herr_t
// H5VL_dtio_group_get(void *obj, H5VL_group_get_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL GROUP Get\n");
// #endif

//     ret_value = H5VLgroup_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_group_get() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_group_specific
//  *
//  * Purpose:     Specific operation on a group
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_group_specific(void *obj, H5VL_group_specific_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     hid_t                under_vol_id;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL GROUP Specific\n");
// #endif

//     /* Save copy of underlying VOL connector ID, in case of
//      * 'refresh' operation destroying the current object
//      */
//     under_vol_id = o->under_vol_id;

//     /* Unpack arguments to get at the child file pointer when mounting a file */
//     if (args->op_type == H5VL_GROUP_MOUNT) {
//         H5VL_group_specific_args_t vol_cb_args; /* New group specific arg struct */

//         /* Set up new VOL callback arguments */
//         vol_cb_args.op_type         = H5VL_GROUP_MOUNT;
//         vol_cb_args.args.mount.name = args->args.mount.name;
//         vol_cb_args.args.mount.child_file =
//             ((H5VL_dtio_t *)args->args.mount.child_file)->under_object;
//         vol_cb_args.args.mount.fmpl_id = args->args.mount.fmpl_id;

//         /* Re-issue 'group specific' call, using the unwrapped pieces */
//         ret_value = H5VLgroup_specific(o->under_object, under_vol_id, &vol_cb_args, dxpl_id, req);
//     } /* end if */
//     else
//         ret_value = H5VLgroup_specific(o->under_object, under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_group_specific() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_group_optional
//  *
//  * Purpose:     Perform a connector-specific operation on a group
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_group_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL GROUP Optional\n");
// #endif

//     ret_value = H5VLgroup_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_group_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_group_close
 *
 * Purpose:     Closes a group.
 *
 * Return:      Success:    0
 *              Failure:    -1, group not closed.
 *
 *-------------------------------------------------------------------------
 */
// static herr_t
// H5VL_dtio_group_close(void *grp, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)grp;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL H5Gclose\n");
// #endif

//     ret_value = H5VLgroup_close(o->under_object, o->under_vol_id, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     /* Release our wrapper, if underlying file was closed */
//     if (ret_value >= 0)
//         H5VL_dtio_free_obj(o);

//     return ret_value;
// } /* end H5VL_dtio_group_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dtio_link_create
 *
 * Purpose:     Creates a hard / soft / UD / external link.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
// static herr_t
// H5VL_dtio_link_create(H5VL_link_create_args_t *args, void *obj, const H5VL_loc_params_t *loc_params,
//                               hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o            = (H5VL_dtio_t *)obj;
//     hid_t                under_vol_id = -1;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL LINK Create\n");
// #endif

//     /* Try to retrieve the "under" VOL id */
//     if (o)
//         under_vol_id = o->under_vol_id;

//     /* Fix up the link target object for hard link creation */
//     if (H5VL_LINK_CREATE_HARD == args->op_type) {
//         void *cur_obj = args->args.hard.curr_obj;

//         /* If cur_obj is a non-NULL pointer, find its 'under object' and update the pointer */
//         if (cur_obj) {
//             /* Check if we still haven't set the "under" VOL ID */
//             if (under_vol_id < 0)
//                 under_vol_id = ((H5VL_dtio_t *)cur_obj)->under_vol_id;

//             /* Update the object for the link target */
//             args->args.hard.curr_obj = ((H5VL_dtio_t *)cur_obj)->under_object;
//         } /* end if */
//     }     /* end if */

//     ret_value = H5VLlink_create(args, (o ? o->under_object : NULL), loc_params, under_vol_id, lcpl_id,
//                                 lapl_id, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_link_create() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_link_copy
//  *
//  * Purpose:     Renames an object within an HDF5 container and copies it to a new
//  *              group.  The original name SRC is unlinked from the group graph
//  *              and then inserted with the new name DST (which can specify a
//  *              new path for the object) as an atomic operation. The names
//  *              are interpreted relative to SRC_LOC_ID and
//  *              DST_LOC_ID, which are either file IDs or group ID.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_link_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
//                             const H5VL_loc_params_t *loc_params2, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id,
//                             void **req)
// {
//     H5VL_dtio_t *o_src        = (H5VL_dtio_t *)src_obj;
//     H5VL_dtio_t *o_dst        = (H5VL_dtio_t *)dst_obj;
//     hid_t                under_vol_id = -1;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL LINK Copy\n");
// #endif

//     /* Retrieve the "under" VOL id */
//     if (o_src)
//         under_vol_id = o_src->under_vol_id;
//     else if (o_dst)
//         under_vol_id = o_dst->under_vol_id;
//     assert(under_vol_id > 0);

//     ret_value =
//         H5VLlink_copy((o_src ? o_src->under_object : NULL), loc_params1, (o_dst ? o_dst->under_object : NULL),
//                       loc_params2, under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_link_copy() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_link_move
//  *
//  * Purpose:     Moves a link within an HDF5 file to a new group.  The original
//  *              name SRC is unlinked from the group graph
//  *              and then inserted with the new name DST (which can specify a
//  *              new path for the object) as an atomic operation. The names
//  *              are interpreted relative to SRC_LOC_ID and
//  *              DST_LOC_ID, which are either file IDs or group ID.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_link_move(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
//                             const H5VL_loc_params_t *loc_params2, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id,
//                             void **req)
// {
//     H5VL_dtio_t *o_src        = (H5VL_dtio_t *)src_obj;
//     H5VL_dtio_t *o_dst        = (H5VL_dtio_t *)dst_obj;
//     hid_t                under_vol_id = -1;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL LINK Move\n");
// #endif

//     /* Retrieve the "under" VOL id */
//     if (o_src)
//         under_vol_id = o_src->under_vol_id;
//     else if (o_dst)
//         under_vol_id = o_dst->under_vol_id;
//     assert(under_vol_id > 0);

//     ret_value =
//         H5VLlink_move((o_src ? o_src->under_object : NULL), loc_params1, (o_dst ? o_dst->under_object : NULL),
//                       loc_params2, under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_link_move() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_link_get
//  *
//  * Purpose:     Get info about a link
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_link_get(void *obj, const H5VL_loc_params_t *loc_params, H5VL_link_get_args_t *args,
//                            hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL LINK Get\n");
// #endif

//     ret_value = H5VLlink_get(o->under_object, loc_params, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_link_get() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_link_specific
//  *
//  * Purpose:     Specific operation on a link
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_link_specific(void *obj, const H5VL_loc_params_t *loc_params,
//                                 H5VL_link_specific_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL LINK Specific\n");
// #endif

//     ret_value = H5VLlink_specific(o->under_object, loc_params, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_link_specific() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_link_optional
//  *
//  * Purpose:     Perform a connector-specific operation on a link
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_link_optional(void *obj, const H5VL_loc_params_t *loc_params, H5VL_optional_args_t *args,
//                                 hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL LINK Optional\n");
// #endif

//     ret_value = H5VLlink_optional(o->under_object, loc_params, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_link_optional() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_object_open
//  *
//  * Purpose:     Opens an object inside a container.
//  *
//  * Return:      Success:    Pointer to object
//  *              Failure:    NULL
//  *
//  *-------------------------------------------------------------------------
//  */
// static void *
// H5VL_dtio_object_open(void *obj, const H5VL_loc_params_t *loc_params, H5I_type_t *opened_type,
//                               hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *new_obj;
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     void                *under;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL OBJECT Open\n");
// #endif

//     under = H5VLobject_open(o->under_object, loc_params, o->under_vol_id, opened_type, dxpl_id, req);
//     if (under) {
//         new_obj = H5VL_dtio_new_obj(under, o->under_vol_id);

//         /* Check for async request */
//         if (req && *req)
//             *req = H5VL_dtio_new_obj(*req, o->under_vol_id);
//     } /* end if */
//     else
//         new_obj = NULL;

//     return (void *)new_obj;
// } /* end H5VL_dtio_object_open() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_object_copy
//  *
//  * Purpose:     Copies an object inside a container.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_object_copy(void *src_obj, const H5VL_loc_params_t *src_loc_params, const char *src_name,
//                               void *dst_obj, const H5VL_loc_params_t *dst_loc_params, const char *dst_name,
//                               hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o_src = (H5VL_dtio_t *)src_obj;
//     H5VL_dtio_t *o_dst = (H5VL_dtio_t *)dst_obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL OBJECT Copy\n");
// #endif

//     ret_value =
//         H5VLobject_copy(o_src->under_object, src_loc_params, src_name, o_dst->under_object, dst_loc_params,
//                         dst_name, o_src->under_vol_id, ocpypl_id, lcpl_id, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o_src->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_object_copy() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_object_get
//  *
//  * Purpose:     Get info about an object
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_object_get(void *obj, const H5VL_loc_params_t *loc_params, H5VL_object_get_args_t *args,
//                              hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL OBJECT Get\n");
// #endif

//     ret_value = H5VLobject_get(o->under_object, loc_params, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_object_get() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_object_specific
//  *
//  * Purpose:     Specific operation on an object
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_object_specific(void *obj, const H5VL_loc_params_t *loc_params,
//                                   H5VL_object_specific_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     hid_t                under_vol_id;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL OBJECT Specific\n");
// #endif

//     /* Save copy of underlying VOL connector ID, in case of
//      * 'refresh' operation destroying the current object
//      */
//     under_vol_id = o->under_vol_id;

//     ret_value = H5VLobject_specific(o->under_object, loc_params, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_object_specific() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_object_optional
//  *
//  * Purpose:     Perform a connector-specific operation for an object
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_object_optional(void *obj, const H5VL_loc_params_t *loc_params, H5VL_optional_args_t *args,
//                                   hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL OBJECT Optional\n");
// #endif

//     ret_value = H5VLobject_optional(o->under_object, loc_params, o->under_vol_id, args, dxpl_id, req);

//     /* Check for async request */
//     if (req && *req)
//         *req = H5VL_dtio_new_obj(*req, o->under_vol_id);

//     return ret_value;
// } /* end H5VL_dtio_object_optional() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_introspect_get_conn_cls
//  *
//  * Purpose:     Query the connector class.
//  *
//  * Return:      SUCCEED / FAIL
//  *
//  *-------------------------------------------------------------------------
//  */
// herr_t
// H5VL_dtio_introspect_get_conn_cls(void *obj, H5VL_get_conn_lvl_t lvl, const H5VL_class_t **conn_cls)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL INTROSPECT GetConnCls\n");
// #endif

//     /* Check for querying this connector's class */
//     if (H5VL_GET_CONN_LVL_CURR == lvl) {
//         *conn_cls = &H5VL_dtio_g;
//         ret_value = 0;
//     } /* end if */
//     else
//         ret_value = H5VLintrospect_get_conn_cls(o->under_object, o->under_vol_id, lvl, conn_cls);

//     return ret_value;
// } /* end H5VL_dtio_introspect_get_conn_cls() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_introspect_get_cap_flags
//  *
//  * Purpose:     Query the capability flags for this connector and any
//  *              underlying connector(s).
//  *
//  * Return:      SUCCEED / FAIL
//  *
//  *-------------------------------------------------------------------------
//  */
// herr_t
// H5VL_dtio_introspect_get_cap_flags(const void *_info, uint64_t *cap_flags)
// {
//     const H5VL_dtio_info_t *info = (const H5VL_dtio_info_t *)_info;
//     herr_t                          ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL INTROSPECT GetCapFlags\n");
// #endif

//     /* Make sure the underneath VOL of this pass-through VOL is specified */
//     if (!info) {
//         printf("\nH5VLdtio.c line %d in %s: info for pass-through VOL can't be null\n", __LINE__,
//                __func__);
//         return -1;
//     }

//     if (H5Iis_valid(info->under_vol_id) <= 0) {
//         printf("\nH5VLdtio.c line %d in %s: not a valid underneath VOL ID for pass-through VOL\n",
//                __LINE__, __func__);
//         return -1;
//     }

//     /* Invoke the query on the underlying VOL connector */
//     ret_value = H5VLintrospect_get_cap_flags(info->under_vol_info, info->under_vol_id, cap_flags);

//     /* Bitwise OR our capability flags in */
//     if (ret_value >= 0)
//         *cap_flags |= H5VL_dtio_g.cap_flags;

//     return ret_value;
// } /* end H5VL_dtio_introspect_get_cap_flags() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_introspect_opt_query
//  *
//  * Purpose:     Query if an optional operation is supported by this connector
//  *
//  * Return:      SUCCEED / FAIL
//  *
//  *-------------------------------------------------------------------------
//  */
// herr_t
// H5VL_dtio_introspect_opt_query(void *obj, H5VL_subclass_t cls, int opt_type, uint64_t *flags)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL INTROSPECT OptQuery\n");
// #endif

//     ret_value = H5VLintrospect_opt_query(o->under_object, o->under_vol_id, cls, opt_type, flags);

//     return ret_value;
// } /* end H5VL_dtio_introspect_opt_query() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_request_wait
//  *
//  * Purpose:     Wait (with a timeout) for an async operation to complete
//  *
//  * Note:        Releases the request if the operation has completed and the
//  *              connector callback succeeds
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_request_wait(void *obj, uint64_t timeout, H5VL_request_status_t *status)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL REQUEST Wait\n");
// #endif

//     ret_value = H5VLrequest_wait(o->under_object, o->under_vol_id, timeout, status);

//     if (ret_value >= 0 && *status != H5ES_STATUS_IN_PROGRESS)
//         H5VL_dtio_free_obj(o);

//     return ret_value;
// } /* end H5VL_dtio_request_wait() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_request_notify
//  *
//  * Purpose:     Registers a user callback to be invoked when an asynchronous
//  *              operation completes
//  *
//  * Note:        Releases the request, if connector callback succeeds
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_request_notify(void *obj, H5VL_request_notify_t cb, void *ctx)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL REQUEST Notify\n");
// #endif

//     ret_value = H5VLrequest_notify(o->under_object, o->under_vol_id, cb, ctx);

//     if (ret_value >= 0)
//         H5VL_dtio_free_obj(o);

//     return ret_value;
// } /* end H5VL_dtio_request_notify() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_request_cancel
//  *
//  * Purpose:     Cancels an asynchronous operation
//  *
//  * Note:        Releases the request, if connector callback succeeds
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_request_cancel(void *obj, H5VL_request_status_t *status)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL REQUEST Cancel\n");
// #endif

//     ret_value = H5VLrequest_cancel(o->under_object, o->under_vol_id, status);

//     if (ret_value >= 0)
//         H5VL_dtio_free_obj(o);

//     return ret_value;
// } /* end H5VL_dtio_request_cancel() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_request_specific
//  *
//  * Purpose:     Specific operation on a request
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_request_specific(void *obj, H5VL_request_specific_args_t *args)
// {
//     H5VL_dtio_t *o         = (H5VL_dtio_t *)obj;
//     herr_t               ret_value = -1;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL REQUEST Specific\n");
// #endif

//     ret_value = H5VLrequest_specific(o->under_object, o->under_vol_id, args);

//     return ret_value;
// } /* end H5VL_dtio_request_specific() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_request_optional
//  *
//  * Purpose:     Perform a connector-specific operation for a request
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_request_optional(void *obj, H5VL_optional_args_t *args)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL REQUEST Optional\n");
// #endif

//     ret_value = H5VLrequest_optional(o->under_object, o->under_vol_id, args);

//     return ret_value;
// } /* end H5VL_dtio_request_optional() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_request_free
//  *
//  * Purpose:     Releases a request, allowing the operation to complete without
//  *              application tracking
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *-------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_request_free(void *obj)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL REQUEST Free\n");
// #endif

//     ret_value = H5VLrequest_free(o->under_object, o->under_vol_id);

//     if (ret_value >= 0)
//         H5VL_dtio_free_obj(o);

//     return ret_value;
// } /* end H5VL_dtio_request_free() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_blob_put
//  *
//  * Purpose:     Handles the blob 'put' callback
//  *
//  * Return:      SUCCEED / FAIL
//  *
//  *-------------------------------------------------------------------------
//  */
// herr_t
// H5VL_dtio_blob_put(void *obj, const void *buf, size_t size, void *blob_id, void *ctx)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL BLOB Put\n");
// #endif

//     ret_value = H5VLblob_put(o->under_object, o->under_vol_id, buf, size, blob_id, ctx);

//     return ret_value;
// } /* end H5VL_dtio_blob_put() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_blob_get
//  *
//  * Purpose:     Handles the blob 'get' callback
//  *
//  * Return:      SUCCEED / FAIL
//  *
//  *-------------------------------------------------------------------------
//  */
// herr_t
// H5VL_dtio_blob_get(void *obj, const void *blob_id, void *buf, size_t size, void *ctx)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL BLOB Get\n");
// #endif

//     ret_value = H5VLblob_get(o->under_object, o->under_vol_id, blob_id, buf, size, ctx);

//     return ret_value;
// } /* end H5VL_dtio_blob_get() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_blob_specific
//  *
//  * Purpose:     Handles the blob 'specific' callback
//  *
//  * Return:      SUCCEED / FAIL
//  *
//  *-------------------------------------------------------------------------
//  */
// herr_t
// H5VL_dtio_blob_specific(void *obj, void *blob_id, H5VL_blob_specific_args_t *args)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL BLOB Specific\n");
// #endif

//     ret_value = H5VLblob_specific(o->under_object, o->under_vol_id, blob_id, args);

//     return ret_value;
// } /* end H5VL_dtio_blob_specific() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_blob_optional
//  *
//  * Purpose:     Handles the blob 'optional' callback
//  *
//  * Return:      SUCCEED / FAIL
//  *
//  *-------------------------------------------------------------------------
//  */
// herr_t
// H5VL_dtio_blob_optional(void *obj, void *blob_id, H5VL_optional_args_t *args)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL BLOB Optional\n");
// #endif

//     ret_value = H5VLblob_optional(o->under_object, o->under_vol_id, blob_id, args);

//     return ret_value;
// } /* end H5VL_dtio_blob_optional() */

// /*---------------------------------------------------------------------------
//  * Function:    H5VL_dtio_token_cmp
//  *
//  * Purpose:     Compare two of the connector's object tokens, setting
//  *              *cmp_value, following the same rules as strcmp().
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *---------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_token_cmp(void *obj, const H5O_token_t *token1, const H5O_token_t *token2, int *cmp_value)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL TOKEN Compare\n");
// #endif

//     /* Sanity checks */
//     assert(obj);
//     assert(token1);
//     assert(token2);
//     assert(cmp_value);

//     ret_value = H5VLtoken_cmp(o->under_object, o->under_vol_id, token1, token2, cmp_value);

//     return ret_value;
// } /* end H5VL_dtio_token_cmp() */

// /*---------------------------------------------------------------------------
//  * Function:    H5VL_dtio_token_to_str
//  *
//  * Purpose:     Serialize the connector's object token into a string.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *---------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_token_to_str(void *obj, H5I_type_t obj_type, const H5O_token_t *token, char **token_str)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL TOKEN To string\n");
// #endif

//     /* Sanity checks */
//     assert(obj);
//     assert(token);
//     assert(token_str);

//     ret_value = H5VLtoken_to_str(o->under_object, obj_type, o->under_vol_id, token, token_str);

//     return ret_value;
// } /* end H5VL_dtio_token_to_str() */

// /*---------------------------------------------------------------------------
//  * Function:    H5VL_dtio_token_from_str
//  *
//  * Purpose:     Deserialize the connector's object token from a string.
//  *
//  * Return:      Success:    0
//  *              Failure:    -1
//  *
//  *---------------------------------------------------------------------------
//  */
// static herr_t
// H5VL_dtio_token_from_str(void *obj, H5I_type_t obj_type, const char *token_str, H5O_token_t *token)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL TOKEN From string\n");
// #endif

//     /* Sanity checks */
//     assert(obj);
//     assert(token);
//     assert(token_str);

//     ret_value = H5VLtoken_from_str(o->under_object, obj_type, o->under_vol_id, token_str, token);

//     return ret_value;
// } /* end H5VL_dtio_token_from_str() */

// /*-------------------------------------------------------------------------
//  * Function:    H5VL_dtio_optional
//  *
//  * Purpose:     Handles the generic 'optional' callback
//  *
//  * Return:      SUCCEED / FAIL
//  *
//  *-------------------------------------------------------------------------
//  */
// herr_t
// H5VL_dtio_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req)
// {
//     H5VL_dtio_t *o = (H5VL_dtio_t *)obj;
//     herr_t               ret_value;

// #ifdef ENABLE_DTIO_LOGGING
//     printf("------- DTIO VOL generic Optional\n");
// #endif

//     ret_value = H5VLoptional(o->under_object, o->under_vol_id, args, dxpl_id, req);

//     return ret_value;
// } /* end H5VL_dtio_optional() */

/* Introspection routines */
static herr_t
H5VL_dtio_introspect_get_conn_cls(void *obj, H5VL_get_conn_lvl_t lvl, const H5VL_class_t **conn_cls)
{
  FUNC_ENTER_VOL(herr_t, SUCCEED)
    
  if (!obj)
    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "item parameter not supplied");
  if (!conn_cls)
    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "conn_cls parameter not supplied");

  *conn_cls = &H5VL_dtio_g;

 done:
  FUNC_LEAVE_VOL
}

static herr_t
H5VL_dtio_introspect_get_cap_flags(const void *info, uint64_t *cap_flags)
{
  FUNC_ENTER_VOL(herr_t, SUCCEED)

  if (!cap_flags)
    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid cap_flags parameter");

  *cap_flags = H5VL_dtio_g.cap_flags;

 done:
  FUNC_LEAVE_VOL
}

static herr_t
H5VL_dtio_introspect_opt_query(void *obj, H5VL_subclass_t cls, int opt_type,
			       uint64_t *supported)
{
  FUNC_ENTER_VOL(herr_t, SUCCEED)

  if (!obj)
    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "\"item\" parameter not supplied");
  if (!supported)
    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "output parameter not supplied");

  switch(opt_type) {
  default: {
    // Currently, we don't claim to support anything
    *supported = 0;
    break;
  }
  }

 done:
  FUNC_LEAVE_VOL
}

H5PL_type_t H5PLget_plugin_type(void) {return H5PL_TYPE_VOL;}
const void *H5PLget_plugin_info(void) {return &H5VL_dtio_g;}
