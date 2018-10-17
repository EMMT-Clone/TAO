/*
 * tao-private.h --
 *
 * Private definitions for compiling TAO.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#ifndef _TAO_PRIVATE_H_
#define _TAO_PRIVATE_H_ 1

#if __STDC_VERSION__ < 199901L
#   error C99 standard must be supported to compile TAO
#   define inline static
#endif

#include "tao.h"
#include "config.h"

#define TAO_ASSERT(expr, code)                                          \
    do {                                                                \
        if (!(expr)) {                                                  \
            tao_push_error(errs, __func__, TAO_ASSERTION_FAILED);       \
            return code;                                                \
        }                                                               \
    } while (0)

/**
 * Check whether any errors occured.
 *
 * @param errs   Address of variable to track errors.
 *
 * @return A boolean result.
 */
#define TAO_ANY_ERRORS(errs) ((errs) != NULL && *(errs) != TAO_NO_ERRORS)

/**
 * Get the offset of a member in a structure.
 *
 * @param type    Structure type.
 * @param member  Structure member.
 *
 * @return A number of bytes.
 */
#define TAO_OFFSET_OF(type, member) ((char*)&((type*)0)->member - (char*)0)

/**
 * Round-up `a` by chuncks of size `b`.
 */
#define TAO_ROUND_UP(a, b)  ((((a) + ((b) - 1))/(b))*(b))

/**
 * Private structure to store a shared array.
 *
 * The definition of this structure is exposed in `"tao-private.h"` but its
 * members should be considered as read-only.  It is recommended to use the
 * public API to manipulate a shared array.
 */
struct tao_shared_array {
    tao_shared_object_t base;     /**< Shared object backing storage of the
                                       shared array */
    tao_element_type_t  eltype;   /**< Type of the elements of the shared
                                       array */
    size_t nelem;                 /**< Number of elements */
    uint32_t ndims;               /**< Number of dimensions */
    uint32_t size[TAO_MAX_NDIMS]; /**< Length of each dimension (dimensions
                                       beyong `ndims` are assumed to be `1`) */
    int32_t nwriters;             /**< Number of writers */
    int32_t nreaders;             /**< Number of readers */
    int64_t counter;              /**< Counter (used for acquired images) */
    int64_t ts_sec;               /**< Time stamp (seconds part) */
    int64_t ts_nsec;              /**< Time stamp (nanoseconds part) */
    uint64_t data[1];             /**< Shared array data (actual size is large
                                       enough to store all pixels, type is to
                                       force correct alignment whatever the
                                       actual element type) */
};

#endif /* _TAO_PRIVATE_H_ */
