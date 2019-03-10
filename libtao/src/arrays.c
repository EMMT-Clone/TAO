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
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

#include "common.h"
#include "tao-private.h"

#include <limits.h>
#include <string.h>

/*---------------------------------------------------------------------------*/

size_t
tao_get_element_size(int eltype)
{
    switch (eltype) {
    case TAO_INT8:    return sizeof(int8_t);
    case TAO_UINT8:   return sizeof(uint8_t);
    case TAO_INT16:   return sizeof(int16_t);
    case TAO_UINT16:  return sizeof(uint16_t);
    case TAO_INT32:   return sizeof(int32_t);
    case TAO_UINT32:  return sizeof(uint32_t);
    case TAO_INT64:   return sizeof(int64_t);
    case TAO_UINT64:  return sizeof(uint64_t);
    case TAO_FLOAT32: return sizeof(float);
    case TAO_FLOAT64: return sizeof(double);
    default:          return 0;
    }
}

long
tao_count_elements(tao_error_t** errs, int ndims, const long dims[])
{
    if (ndims < 0 || ndims > TAO_MAX_NDIMS) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return 0;
    }
    if (ndims > 0 && dims == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return 0;
    }
    long nelem = 1;
    for (int d = 0; d < ndims; ++d) {
        long dim = dims[d];
        if (dim <= 0) {
            tao_push_error(errs, __func__, TAO_BAD_SIZE);
            return 0;
        }
        if (dim > 1) {
            /* Count number of elements and check for overflows. */
            if (nelem > LONG_MAX/dim) {
                tao_push_error(errs, __func__, TAO_BAD_SIZE);
                return 0;
            }
            nelem *= dim;
        }
    }
    return nelem;
}

tao_array_t*
tao_create_array(tao_error_t** errs, tao_element_type_t eltype,
                 int ndims, const long dims[])
{
    long nelem = tao_count_elements(errs, ndims, dims);
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
    for (int d = ndims; d < TAO_MAX_NDIMS; ++d) {
        arr->dims[d] = 1;
    }
    size_t address = (char*)arr - (char*)0;
    arr->data = (void*)TAO_ROUND_UP(address + header, ALIGNMENT);
    return arr;
}

tao_array_t*
tao_wrap_array(tao_error_t** errs, tao_element_type_t eltype,
               int ndims, const long dims[], void* data,
               void (*free)(void*), void* ctx)
{
    long nelem = tao_count_elements(errs, ndims, dims);
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
    for (int d = ndims; d < TAO_MAX_NDIMS; ++d) {
        arr->dims[d] = 1;
    }
    arr->data = data;
    arr->free = free;
    arr->ctx = ctx;
    return arr;
}

tao_array_t*
tao_create_1d_array(tao_error_t** errs, tao_element_type_t eltype,
                    long dim1)
{
    long dims[1];
    dims[0] = dim1;
    return tao_create_array(errs, eltype, 1, dims);
}

tao_array_t*
tao_create_2d_array(tao_error_t** errs, tao_element_type_t eltype,
                    long dim1, long dim2)
{
    long dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return tao_create_array(errs, eltype, 2, dims);
}

tao_array_t*
tao_create_3d_array(tao_error_t** errs, tao_element_type_t eltype,
                    long dim1, long dim2, long dim3)
{
    long dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return tao_create_array(errs, eltype, 3, dims);
}

tao_array_t*
tao_wrap_1d_array(tao_error_t** errs, tao_element_type_t eltype,
                  long dim1, void* data, void (*free)(void*), void* ctx)
{
    long dims[1];
    dims[0] = dim1;
    return tao_wrap_array(errs, eltype, 1, dims, data, free, ctx);
}

tao_array_t*
tao_wrap_2d_array(tao_error_t** errs, tao_element_type_t eltype,
                  long dim1, long dim2, void* data,
                  void (*free)(void*), void* ctx)
{
    long dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return tao_wrap_array(errs, eltype, 2, dims, data, free, ctx);
}

tao_array_t*
tao_wrap_3d_array(tao_error_t** errs, tao_element_type_t eltype,
                  long dim1, long dim2, long dim3, void* data,
                  void (*free)(void*), void* ctx)
{
    long dims[3];
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
    return (TAO_LIKELY(arr != NULL) ? arr->eltype : -1);
}

long
tao_get_array_length(const tao_array_t* arr)
{
    return (TAO_LIKELY(arr != NULL) ? arr->nelem : 0);
}

int
tao_get_array_ndims(const tao_array_t* arr)
{
    return (TAO_LIKELY(arr != NULL) ? arr->ndims : 0);
}

long
tao_get_array_size(const tao_array_t* arr, int d)
{
    return (TAO_LIKELY(arr != NULL) ?
            (TAO_UNLIKELY(d < 1) ? 0 :
             (TAO_UNLIKELY(d > TAO_MAX_NDIMS) ? 1 :
              arr->dims[d-1])) : 0);
}

void*
tao_get_array_data(const tao_array_t* arr)
{
    return (TAO_LIKELY(arr != NULL) ? arr->data : NULL);
}
