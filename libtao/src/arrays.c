/*
 * arrays.c --
 *
 * Multi-dimensional arrays in TAO library (TAO is a library for Adaptive
 * Optics software).
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

#include <string.h>

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
