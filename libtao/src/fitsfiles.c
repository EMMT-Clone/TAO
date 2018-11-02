/*
 * fitsfiles.c --
 *
 * Loading of FITS files for TAO (TAO is a library for Adaptive Optics
 * software).
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include "config.h"
#include "macros.h"
#include "tao-private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static void
get_fits_error_details(int status, const char** reason, const char** info)
{
    if (reason != NULL) {
        *reason = NULL;
    }
    if (info != NULL) {
        *info = NULL;
    }
}

void
tao_push_fits_error(tao_error_t** errs, const char* func, int status)
{
    tao_push_other_error(errs, func, status, get_fits_error_details);
}

/*
 * Yields TAO element type given FITSIO bitpix, -1 in case of error.  Beware
 * that the BITPIX symbolic names used in FITSIO are somewhat misleading (for
 * instance LONG_IMG means 32-bit signed integers, not necessarily C long
 * integer type).
 */
static tao_element_type_t
bitpix_to_element_type(int bitpix)
{

#define FLOAT(n) ((n) == 4 ? TAO_FLOAT32 :              \
                  ((n) == 8 ? TAO_FLOAT64 : -1))

    switch (bitpix) {
    case SBYTE_IMG:    return TAO_INT8;
    case BYTE_IMG:     return TAO_UINT8;
    case SHORT_IMG:    return TAO_INT16;
    case USHORT_IMG:   return TAO_UINT16;
    case LONG_IMG:     return TAO_INT32;
    case ULONG_IMG:    return TAO_UINT32;
    case LONGLONG_IMG: return TAO_INT64;
    case FLOAT_IMG:    return FLOAT(sizeof(float));
    case DOUBLE_IMG:   return FLOAT(sizeof(double));
    default:           return -1;
    }

#undef FLOAT

}

/* Yields FITSIO bitpix given TAO element type, -1 in case of error. */
static int
element_type_to_bitpix(tao_element_type_t eltype)
{

#define FLOAT(n) ((n) == sizeof(float) ? FLOAT_IMG :            \
                  ((n) == sizeof(double) ? DOUBLE_IMG : -1))

    switch (eltype) {
    case TAO_INT8:    return SBYTE_IMG;
    case TAO_UINT8:   return BYTE_IMG;
    case TAO_INT16:   return SHORT_IMG;
    case TAO_UINT16:  return USHORT_IMG;
    case TAO_INT32:   return LONG_IMG;
    case TAO_UINT32:  return ULONG_IMG;
    case TAO_INT64:   return LONGLONG_IMG;
    case TAO_UINT64:  return LONGLONG_IMG; /* hope for the best! */
    case TAO_FLOAT32: return FLOAT(4);
    case TAO_FLOAT64: return FLOAT(8);
    default:          return -1;
    }

#undef FLOAT

}

/* Yields FITSIO datatype given TAO element type, -1 in case of error. */
static int
element_type_to_datatype(tao_element_type_t eltype)
{

#define SIGNED(n)                                       \
    ((n) == 1 ? TSBYTE :                                \
     ((n) == sizeof(short) ? TSHORT :                   \
      ((n) == sizeof(int) ? TINT :                      \
       ((n) == sizeof(long) ? TLONG :                   \
        ((n) == sizeof(LONGLONG) ? TLONGLONG : -1)))))

#define UNSIGNED(n)                             \
    ((n) == 1 ? TBYTE :                         \
     ((n) == sizeof(short) ? TUSHORT :          \
      ((n) == sizeof(int) ? TUINT :             \
       ((n) == sizeof(long) ? TULONG : -1))))

#define FLOAT(n)                                \
    ((n) == sizeof(float) ? TFLOAT :            \
     ((n) == sizeof(double) ? TDOUBLE : -1))

    switch (eltype) {
    case TAO_INT8:    return   SIGNED(1);
    case TAO_UINT8:   return UNSIGNED(1);
    case TAO_INT16:   return   SIGNED(2);
    case TAO_UINT16:  return UNSIGNED(2);
    case TAO_INT32:   return   SIGNED(4);
    case TAO_UINT32:  return UNSIGNED(4);
    case TAO_INT64:   return   SIGNED(8);
    case TAO_UINT64:  return UNSIGNED(8);
    case TAO_FLOAT32: return    FLOAT(4);
    case TAO_FLOAT64: return    FLOAT(8);
    default:          return -1;
    }

#undef FLOAT
#undef UNSIGNED
#undef SIGNED

}

tao_array_t*
tao_load_array_from_fits_file(tao_error_t** errs, const char* filename,
                              char* extname)
{
    int status = 0;
    fitsfile* fptr;
    tao_array_t* arr = NULL;

    fits_clear_errmsg();
    if (extname == NULL) {
        /* Open the FITS file and move to the first IMAGE HDU. */
        fits_open_image(&fptr, filename, READONLY, &status);
    } else {
        /* Open the FITS file and move to the specified IMAGE extension. */
        if (fits_open_diskfile(&fptr, filename, READONLY, &status) == 0) {
            fits_movnam_hdu(fptr, IMAGE_HDU, extname, 0, &status);
        }
    }
    if (status == 0) {
        arr = tao_load_array_from_fits_handle(errs, fptr);
        fits_close_file(fptr, &status);
    }
    if (status != 0) {
        if (arr != NULL) {
            tao_unreference_array(arr);
        }
        tao_push_fits_error(errs, __func__, status);
        return NULL;
    }
    return arr;
}

tao_array_t*
tao_load_array_from_fits_handle(tao_error_t** errs, fitsfile* fptr)
{
    int bitpix, ndims;
    int status = 0;
    long dims[TAO_MAX_NDIMS];

    /* Get the type of the elements. */
    if (fits_get_img_equivtype(fptr, &bitpix, &status) != 0) {
        tao_push_fits_error(errs, "fits_get_img_equivtype", status);
        return NULL;
    }
    tao_element_type_t eltype = bitpix_to_element_type(bitpix);
    int datatype = element_type_to_datatype(eltype);
    if (eltype == -1 || datatype == -1) {
        tao_push_error(errs, __func__, TAO_BAD_TYPE);
        return NULL;
    }

    /* Get the dimensions array. */
    if (fits_get_img_dim(fptr, &ndims, &status) != 0) {
        tao_push_fits_error(errs, "fits_get_img_dim", status);
        return NULL;
    }
    if (ndims > TAO_MAX_NDIMS) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return NULL;
    }
    if (fits_get_img_size(fptr, ndims, dims, &status) != 0) {
        tao_push_fits_error(errs, "fits_get_img_size", status);
        return NULL;
    }

    /* Create the array. */
    tao_array_t* arr = tao_create_array(errs, eltype, ndims, dims);
    if (arr == NULL) {
        return NULL;
    }

    /* Read the data. */
    void* data = tao_get_array_data(arr);
    long nelem = tao_get_array_length(arr);
    int anynull;
    if (fits_read_img(fptr, datatype, 1, nelem,
                      NULL, data, &anynull, &status) != 0) {
        tao_push_fits_error(errs, "fits_read_img", status);
        tao_unreference_array(arr);
        return NULL;
    }
    return arr;
}

int
tao_save_array_to_fits_file(tao_error_t** errs, const tao_array_t* arr,
                            const char* filename, int overwrite)
{
    int code, status = 0;
    fitsfile* fptr;

    fits_clear_errmsg();
    if (! overwrite) {
        /* Check that output file does not exists. */
        int fd = open(filename, O_CREAT|O_EXCL, 0644);
        if (fd == -1) {
            tao_push_error(errs, __func__, TAO_ALREADY_EXIST);
            return -1;
        }
        close(fd);
    }
    unlink(filename);
    if (fits_create_file(&fptr, filename, &status) != 0) {
        tao_push_fits_error(errs, "fits_create_file", status);
        return -1;
    }
    code = tao_save_array_to_fits_handle(errs, arr, fptr, NULL, NULL);
    if (fits_close_file(fptr, &status) != 0) {
        tao_push_fits_error(errs, "fits_close_file", status);
        code = -1;
    }
    return code;
}

int
tao_save_array_to_fits_handle(tao_error_t** errs, const tao_array_t* arr,
                              fitsfile* fptr,
                              int (*callback)(tao_error_t**, fitsfile*, void*),
                              void* ctx)
{
    /* Get BITPIX and datatype from array element type. */
    if (arr == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    tao_element_type_t eltype = tao_get_array_eltype(arr);
    int bitpix = element_type_to_bitpix(eltype);
    int datatype = element_type_to_datatype(tao_get_array_eltype(arr));
    if (bitpix == -1 || datatype == -1) {
        tao_push_error(errs, __func__, TAO_BAD_TYPE);
        return -1;
    }

    /* Create image HDU. */
    long dims[TAO_MAX_NDIMS];
    int ndims = tao_get_array_ndims(arr);
    for (int d = 1; d <= ndims; ++d) {
        dims[d-1] = tao_get_array_size(arr, d);
    }
    int status = 0;
    if (fits_create_img(fptr, bitpix, ndims, dims, &status) != 0) {
        tao_push_fits_error(errs, "fits_create_img", status);
        return -1;
    }

    /* Update header if requested so. */
    if (callback != NULL) {
        if (callback(errs, fptr, ctx) != 0) {
            return -1;
        }
    }

    /* Write array contents. */
    long nelem = tao_get_array_length(arr);
    void* data = tao_get_array_data(arr);
    if (fits_write_img(fptr, datatype, 1, nelem, data, &status) != 0) {
        tao_push_fits_error(errs, "fits_write_img", status);
        return -1;
    }
    return 0;
}
