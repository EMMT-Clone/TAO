/*
 * sharedarrays.c --
 *
 * Implement multi-dimensional arrays whose contents can be shared between
 * processes.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include "config.h"
#include "macros.h"
#include "tao-private.h"

int
tao_get_shared_array_ident(const tao_shared_array_t* arr)
{
    return (likely(arr != NULL) ? arr->base.ident : -1);
}

int
tao_get_shared_array_eltype(const tao_shared_array_t* arr)
{
    return (likely(arr != NULL) ? arr->eltype : -1);
}

size_t
tao_get_shared_array_length(const tao_shared_array_t* arr)
{
    return (likely(arr != NULL) ? arr->nelem : 0);
}

int
tao_get_shared_array_ndims(const tao_shared_array_t* arr)
{
    return (likely(arr != NULL) ? arr->ndims : 0);
}

size_t
tao_get_shared_array_size(const tao_shared_array_t* arr, int d)
{
    return (likely(arr != NULL) ?
            (unlikely(d < 1) ? 0 :
             (unlikely(d > TAO_MAX_NDIMS) ? 1 :
              arr->size[d-1])) : 0);
}

void*
tao_get_shared_array_data(const tao_shared_array_t* arr)
{
    return (likely(arr != NULL) ?
            (void*)((uint8_t*)arr + arr->offset) : (void*)0);
}

int
tao_get_shared_array_nreaders(const tao_shared_array_t* arr)
{
    return (likely(arr != NULL) ? arr->nreaders : 0);
}

int
tao_adjust_shared_array_nreaders(tao_shared_array_t* arr, int adj)
{
    return (likely(arr != NULL) ? (arr->nreaders += adj) : 0);
}

int
tao_get_shared_array_nwriters(const tao_shared_array_t* arr)
{
    return (likely(arr != NULL) ? arr->nwriters : 0);
}

int
tao_adjust_shared_array_nwriters(tao_shared_array_t* arr, int adj)
{
    return (likely(arr != NULL) ? (arr->nwriters += adj) : 0);
}

int64_t
tao_get_shared_array_counter(const tao_shared_array_t* arr)
{
    return (likely(arr != NULL) ? arr->counter : -1);
}

void
tao_set_shared_array_counter(tao_shared_array_t* arr, int64_t cnt)
{
    if (likely(arr != NULL)) {
        arr->counter = cnt;
    }
}

void
tao_get_shared_array_timestamp(const tao_shared_array_t* arr,
                               int64_t* ts_sec, int64_t* ts_nsec)
{
    if (likely(arr != NULL)) {
        *ts_sec = arr->ts_sec;
        *ts_nsec = arr->ts_nsec;
    } else {
        *ts_sec = -1;
        *ts_nsec = 0;
    }
}

void
tao_set_shared_array_timestamp(tao_shared_array_t* arr,
                               int64_t ts_sec, int64_t ts_nsec)
{
    if (likely(arr != NULL)) {
        arr->ts_sec = ts_sec;
        arr->ts_nsec = ts_nsec;
    }
}

tao_shared_array_t*
tao_create_1d_shared_array(tao_error_t** errs, tao_element_type_t eltype,
                           size_t dim1, unsigned int perms)
{
    size_t size[1];
    size[0] = dim1;
    return tao_create_shared_array(errs, eltype, 1, size, perms);
}

tao_shared_array_t*
tao_create_2d_shared_array(tao_error_t** errs, tao_element_type_t eltype,
                           size_t dim1, size_t dim2, unsigned int perms)
{
    size_t size[2];
    size[0] = dim1;
    size[1] = dim2;
    return tao_create_shared_array(errs, eltype, 2, size, perms);
}

tao_shared_array_t*
tao_create_3d_shared_array(tao_error_t** errs, tao_element_type_t eltype,
                           size_t dim1, size_t dim2, size_t dim3,
                           unsigned int perms)
{
    size_t size[3];
    size[0] = dim1;
    size[1] = dim2;
    size[2] = dim3;
    return tao_create_shared_array(errs, eltype, 3, size, perms);
}

tao_shared_array_t*
tao_create_shared_array(tao_error_t** errs, tao_element_type_t eltype,
                        int ndims, const size_t size[], unsigned int perms)
{
    size_t elsize = tao_get_element_size(eltype);
    if (elsize < 1) {
        tao_push_error(errs, __func__, TAO_BAD_TYPE);
        return NULL;
    }
    size_t nelem = tao_count_elements(errs, ndims, size);
    if (nelem < 1) {
        return NULL;
    }
    for (int d = 0; d < ndims; ++d) {
        if (size[d] <= 0 || size[d] > UINT32_MAX) {
            tao_push_error(errs, __func__, TAO_BAD_SIZE);
            return NULL;
        }
    }
    size_t offset = TAO_ROUND_UP(sizeof(tao_shared_array_t), ALIGNMENT);
    size_t nbytes = offset + nelem*elsize;
    tao_shared_object_t* obj = tao_create_shared_object(errs, TAO_SHARED_ARRAY,
                                                        nbytes, perms);
    if (obj == NULL) {
        return NULL;
    }
    tao_shared_array_t* arr = (tao_shared_array_t*)obj;
    arr->offset = offset;
    arr->nelem = nelem;
    arr->ndims = ndims;
    for (int d = 0; d < ndims; ++d) {
        arr->size[d] = size[d];
    }
    for (int d = ndims; d < TAO_MAX_NDIMS; ++d) {
        arr->size[d] = 1;
    }
    arr->eltype = eltype;
    arr->nwriters = 0;
    arr->nreaders = 0;
    arr->counter = 0;
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        /* Just ignore this error. */
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
    }
    arr->ts_sec = ts.tv_sec;
    arr->ts_nsec = ts.tv_nsec;
    return arr;
}

tao_shared_array_t*
tao_attach_shared_array(tao_error_t** errs, int ident)
{
    return (tao_shared_array_t*)tao_attach_shared_object(errs, ident,
                                                         TAO_SHARED_ARRAY);
}

int
tao_detach_shared_array(tao_error_t** errs, tao_shared_array_t* arr)
{
    /* No needs to check address. */
    return tao_detach_shared_object(errs, (tao_shared_object_t*)arr);
}

int
tao_lock_shared_array(tao_error_t** errs, tao_shared_array_t* arr)
{
    if (unlikely(arr == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return tao_lock_mutex(errs, &arr->base.mutex);
}

int
tao_try_lock_shared_array(tao_error_t** errs, tao_shared_array_t* arr)
{
    if (unlikely(arr == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return tao_try_lock_mutex(errs, &arr->base.mutex);
}

int
tao_unlock_shared_array(tao_error_t** errs, tao_shared_array_t* arr)
{
    if (unlikely(arr == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return tao_unlock_mutex(errs, &arr->base.mutex);
}
