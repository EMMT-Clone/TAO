/*
 * common.h --
 *
 * Common definitions for compiling TAO library.  This file should be included
 * *before* other headers and should be the only one which includes "config.h".
 * Since this file includes "config.h", it is not meant to be installed (unlike
 * "tao.h" or "tao-*.h")
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

#ifndef _TAO_COMMON_H
#define _TAO_COMMON_H 1

#include "config.h"

#define float32_t  float
#define float64_t  double

#ifndef __cplusplus
#  ifdef HAVE_STDBOOL_H
#     include <stdbool.h>
#  else
/*
 * Provide definitions normally in <stdbool.h> (see
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/stdbool.h.html)
 */
#     define bool    _Bool
#     define true    1
#     define false   0
#     define __bool_true_false_are_defined 1
/*
 * _Bool is a keyword of the C language as of C99.  The standard says that it
 * is an unsigned integer type.
 */
#    if ! defined(__STDC_VERSION__) || __STDC_VERSION__ <= 199901L
typedef insigned int _Bool;
#    endif
#  endif
#endif

/*
 * Alignment of data for vectorization depends on the chosen compilation
 * settings.  The following table summarizes the value of macro
 * `__BIGGEST_ALIGNMENT__` with different settings:
 *
 * ---------------------------------------
 * Alignment (bytes)   Compilation Options
 * ---------------------------------------
 *      16             -ffast-math -msse
 *      16             -ffast-math -msse2
 *      16             -ffast-math -msse3
 *      16             -ffast-math -msse4
 *      16             -ffast-math -mavx
 *      32             -ffast-math -mavx2
 * ---------------------------------------
 *
 * The address of attached shared memory is a multiple of memory page size
 * (PAGE_SIZE which is 4096 on the Linux machine I tested) and so much
 * larger than ALIGNMENT (defined below).  So, in principle, it is sufficient
 * to align the shared array data to a multiple of ALIGNMENT relative to the
 * address of the attached shared memory to have correct alignment for all
 * processes.
 */
#define ALIGNMENT 32
#if defined(__BIGGEST_ALIGNMENT__) && __BIGGEST_ALIGNMENT__ > ALIGNMENT
#  undef ALIGNMENT
#  define ALIGNMENT __BIGGEST_ALIGNMENT__
#endif

#endif /* _TAO_COMMON_H */
