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

#include "tao-private.h"

struct tao_shared_array {
    tao_shared_object_t base;     /**< Shared object backing storage of the
                                   *   shared array */
    tao_element_type_t  eltype;   /**< Type of the elements of the shared
                                   *   array */
    size_t nelem;                 /**< Number of elements */
    uint32_t ndims;               /**< Number of dimensions */
    uint32_t size[TAO_MAX_NDIMS]; /**< Length of each dimension (dimensions
                                   *   beyong `ndims` are assumed to be `1`) */
    uint64_t data[1];             /**< Shared array data (actual size is large
                                   *   enough to store all pixels, type is to
                                   *   force correct alignment whatever the
                                   *   actual element type) */
};

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

int
tao_get_array_ident(const tao_shared_array_t* arr)
{
    return arr->base.ident;
}

int
tao_get_array_eltype(const tao_shared_array_t* arr)
{
    return arr->eltype;
}

size_t
tao_get_array_length(const tao_shared_array_t* arr)
{
    return arr->nelem;
}

int
tao_get_array_ndims(const tao_shared_array_t* arr)
{
    return arr->ndims;
}

size_t
tao_get_array_size(const tao_shared_array_t* arr, int d)
{
    return (d < 1 ? 0 : (d > TAO_MAX_NDIMS ? 1 : arr->size[d-1]));
}

void*
tao_get_array_data(const tao_shared_array_t* arr)
{
    return (void*)&arr->data[0];
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
    if (ndims < 0 || ndims > TAO_MAX_NDIMS) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return NULL;
    }
    size_t nelem = 1;
    for (int d = 0; d < ndims; ++d) {
        /* FIXME: check for overflows */
        if (size[d] <= 0) {
            tao_push_error(errs, __func__, TAO_BAD_SIZE);
            return NULL;
        }
        nelem *= size[d];
    }
    size_t nbytes = TAO_OFFSET_OF(tao_shared_array_t, data) + nelem*elsize;
    tao_shared_object_t* obj = tao_create_shared_object(errs, TAO_SHARED_ARRAY,
                                                        nbytes, perms);
    if (obj == NULL) {
        return NULL;
    }
    tao_shared_array_t* arr = (tao_shared_array_t*)obj;
    arr->eltype = eltype;
    arr->nelem = nelem;
    arr->ndims = ndims;
    for (int d = 0; d < ndims; ++d) {
        arr->size[d] = size[d];
    }
    for (int d = ndims; d < TAO_MAX_NDIMS; ++d) {
        arr->size[d] = 1;
    }
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
    return tao_detach_shared_object(errs, (tao_shared_object_t*)arr);
}

int
tao_lock_shared_array(tao_error_t** errs, tao_shared_array_t* arr)
{
    return tao_lock_mutex(errs, &arr->base.mutex);
}

int
tao_try_lock_shared_array(tao_error_t** errs, tao_shared_array_t* arr)
{
    return tao_try_lock_mutex(errs, &arr->base.mutex);
}

int
tao_unlock_shared_array(tao_error_t** errs, tao_shared_array_t* arr)
{
    return tao_unlock_mutex(errs, &arr->base.mutex);
}
