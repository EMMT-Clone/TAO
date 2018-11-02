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

/**
 * Private structure to store an array.
 *
 * The definition of this structure is exposed in `"tao-private.h"` but its
 * members should be considered as read-only.  It is recommended to use the
 * public API to manipulate a shared array.
 */
struct tao_array {
    int nrefs;                  /**< Number of references on the object */
    int ndims;                  /**< Number of dimensions */
    void* data;                 /**< Address of first array element */
    size_t nelem;               /**< Number of elements */
    size_t dims[TAO_MAX_NDIMS]; /**< Length of each dimension (dimensions
                                     beyong `ndims` are assumed to be `1`) */
    tao_element_type_t  eltype; /**< Type of the elements of the shared
                                     array */
    void (*free)(void*);        /**< If non-NULL, function to call to release
                                     data */
    void* ctx;                  /**< Context used as argument to free */
};




/*---------------------------------------------------------------------------*/

#include <string.h>

static size_t
count_array_elements(tao_error_t** errs, int ndims, const size_t dims[])
{
    if (ndims < 0) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return 0;
    }
    if (ndims > 0 && dims == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return 0;
    }
    size_t nelem = 1;
    for (int i = 0; i < ndims; ++i) {
        if (dims[i] <= 0) {
            tao_push_error(errs, __func__, TAO_BAD_SIZE);
            return 0;
        }
        /* FIXME: check for overflows */
        nelem *= dims[i];
    }
    return nelem;
}

tao_array_t*
tao_create_array(tao_error_t** errs, tao_element_type_t eltype,
                 int ndims, const size_t dims[])
{
    size_t nelem = count_array_elements(errs, ndims, dims);
    if (nelem < 1) {
        return NULL;
    }
    size_t elsize = tao_get_element_size(eltype);
    if (elsize < 1) {
        tao_push_error(errs, __func__, TAO_BAD_TYPE);
        return NULL;
    }
    size_t header = sizeof(tao_array_t);
    size_t size = header + ALIGNMENT - 1 + elsize*nelem;
    tao_array_t* arr = (tao_array_t*)tao_malloc(errs, size);
    if (arr == NULL) {
        return NULL;
    }
    memset(arr, 0, sizeof(tao_array_t));
    arr->nrefs = 1;
    arr->eltype = eltype;
    arr->nelem = nelem;
    arr->ndims = ndims;
    for (int i = 0; i < ndims; ++i) {
        arr->dims[i] = dims[i];
    }
    size_t address = (char*)arr - (char*)0;
    arr->data = (void*)TAO_ROUND_UP(address + header, ALIGNMENT);
    return arr;
}

tao_array_t*
tao_wrap_array(tao_error_t** errs, tao_element_type_t eltype,
               int ndims, const size_t dims[], void* data,
               void (*free)(void*), void* ctx)
{
    size_t nelem = count_array_elements(errs, ndims, dims);
    if (nelem < 1) {
        return NULL;
    }
    size_t elsize = tao_get_element_size(eltype);
    if (elsize < 1) {
        tao_push_error(errs, __func__, TAO_BAD_TYPE);
        return NULL;
    }
    tao_array_t* arr = (tao_array_t*)tao_malloc(errs, sizeof(tao_array_t));
    if (arr == NULL) {
        return NULL;
    }
    memset(arr, 0, sizeof(tao_array_t));
    arr->nrefs = 1;
    arr->eltype = eltype;
    arr->nelem = nelem;
    arr->ndims = ndims;
    for (int i = 0; i < ndims; ++i) {
        arr->dims[i] = dims[i];
    }
    arr->data = data;
    arr->free = free;
    arr->ctx = ctx;
    return arr;
}

tao_array_t*
tao_create_1d_array(tao_error_t** errs, tao_element_type_t eltype,
                    size_t dim1)
{
    size_t dims[1];
    dims[0] = dim1;
    return tao_create_array(errs, eltype, 1, dims);
}

tao_array_t*
tao_create_2d_array(tao_error_t** errs, tao_element_type_t eltype,
                    size_t dim1, size_t dim2)
{
    size_t dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return tao_create_array(errs, eltype, 2, dims);
}

tao_array_t*
tao_create_3d_array(tao_error_t** errs, tao_element_type_t eltype,
                    size_t dim1, size_t dim2, size_t dim3)
{
    size_t dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return tao_create_array(errs, eltype, 3, dims);
}

tao_array_t*
tao_wrap_1d_array(tao_error_t** errs, tao_element_type_t eltype,
                  size_t dim1, void* data, void (*free)(void*), void* ctx)
{
    size_t dims[1];
    dims[0] = dim1;
    return tao_wrap_array(errs, eltype, 1, dims, data, free, ctx);
}

tao_array_t*
tao_wrap_2d_array(tao_error_t** errs, tao_element_type_t eltype,
                  size_t dim1, size_t dim2, void* data,
                  void (*free)(void*), void* ctx)
{
    size_t dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return tao_wrap_array(errs, eltype, 2, dims, data, free, ctx);
}

tao_array_t*
tao_wrap_3d_array(tao_error_t** errs, tao_element_type_t eltype,
                  size_t dim1, size_t dim2, size_t dim3, void* data,
                  void (*free)(void*), void* ctx)
{
    size_t dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return tao_wrap_array(errs, eltype, 3, dims, data, free, ctx);
}

tao_array_t*
tao_reference_array(tao_array_t* arr)
{
    ++arr->nrefs;
    return arr;
}

static void
destroy_array(tao_array_t* arr)
{
    if (arr->free != NULL) {
        arr->free(arr->ctx);
    }
    free(arr);
}

void
tao_unreference_array(tao_array_t* arr)
{
    if (--arr->nrefs == 0) {
        destroy_array(arr);
    }
}

tao_element_type_t
tao_get_array_eltype(const tao_array_t* arr)
{
    return (likely(arr != NULL) ? arr->eltype : -1);
}

size_t
tao_get_array_length(const tao_array_t* arr)
{
    return (likely(arr != NULL) ? arr->nelem : 0);
}

int
tao_get_array_ndims(const tao_array_t* arr)
{
    return (likely(arr != NULL) ? arr->ndims : 0);
}

size_t
tao_get_array_size(const tao_array_t* arr, int d)
{
    return (likely(arr != NULL) ?
            (unlikely(d < 1) ? 0 :
             (unlikely(d > TAO_MAX_NDIMS) ? 1 :
              arr->dims[d-1])) : 0);
}

void*
tao_get_array_data(const tao_array_t* arr)
{
    return (likely(arr != NULL) ? arr->data : NULL);
}

/*---------------------------------------------------------------------------*/

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

/* Yields TAO element type given FITSIO bitpix, -1 in case of error. */
static tao_element_type_t
bitpix_to_element_type(int bitpix)
{

#define SIGNED(n) ((n) == 1 ? TAO_INT8 :                \
                   ((n) == 2 ? TAO_INT16 :              \
                    ((n) == 4 ? TAO_INT32 :             \
                     ((n) == 8 ? TAO_INT64 : -1))))

#define UNSIGNED(n) ((n) == 1 ? TAO_UINT8 :             \
                     ((n) == 2 ? TAO_UINT16 :           \
                      ((n) == 4 ? TAO_UINT32 :          \
                       ((n) == 8 ? TAO_UINT64 : -1))))

#define FLOAT(n) ((n) == 4 ? TAO_FLOAT32 :              \
                  ((n) == 8 ? TAO_FLOAT64 : -1))

    switch (bitpix) {
    case BYTE_IMG:     return TAO_UINT8;
    case SBYTE_IMG:    return TAO_INT8;
    case SHORT_IMG:    return SIGNED(sizeof(signed short));
    case USHORT_IMG:   return UNSIGNED(sizeof(unsigned short));
    case LONG_IMG:     return SIGNED(sizeof(signed long));
    case ULONG_IMG:    return UNSIGNED(sizeof(unsigned long));
    case LONGLONG_IMG: return SIGNED(sizeof(LONGLONG));
    case FLOAT_IMG:    return FLOAT(sizeof(float));
    case DOUBLE_IMG:   return FLOAT(sizeof(double));
    default:           return -1;
    }

#undef FLOAT
#undef UNSIGNED
#undef SIGNED

}

/* Yields FITSIO datatype given TAO element type, -1 in case of error. */
static int
element_type_to_datatype(tao_element_type_t eltype)
{
#define SIGNED(n)                                       \
    ((n) == sizeof(short) ? TSHORT :                    \
     ((n) == sizeof(int) ? TINT :                       \
      ((n) == sizeof(long) ? TLONG :                    \
       ((n) == sizeof(LONGLONG) ? TLONGLONG : -1))))

#define UNSIGNED(n)                             \
    ((n) == sizeof(short) ? TUSHORT :           \
     ((n) == sizeof(int) ? TUINT :              \
      ((n) == sizeof(long) ? TULONG : -1)))

#define FLOAT(n)                                \
    ((n) == sizeof(float) ? TFLOAT :            \
     ((n) == sizeof(double) ? TDOUBLE : -1))

    switch (eltype) {
    case TAO_INT8:    return TBYTE;
    case TAO_UINT8:   return TSBYTE;
    case TAO_INT16:   return SIGNED(2);
    case TAO_UINT16:  return UNSIGNED(2);
    case TAO_INT32:   return SIGNED(4);
    case TAO_UINT32:  return UNSIGNED(4);
    case TAO_INT64:   return SIGNED(8);
    case TAO_UINT64:  return UNSIGNED(8);
    case TAO_FLOAT32: return FLOAT(4);
    case TAO_FLOAT64: return FLOAT(8);
    default:           return -1;
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
    long fitsdims[TAO_MAX_NDIMS];
    size_t arrdims[TAO_MAX_NDIMS];

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
    if (fits_get_img_size(fptr, ndims, fitsdims, &status) != 0) {
        tao_push_fits_error(errs, "fits_get_img_size", status);
        return NULL;
    }

    /* Create the array. */
    for (int i = 0; i < ndims; ++i) {
        arrdims[i] = fitsdims[i];
    }
    tao_array_t* arr = tao_create_array(errs, eltype, ndims, arrdims);
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
