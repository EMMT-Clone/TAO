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


#endif /* _TAO_PRIVATE_H_ */
