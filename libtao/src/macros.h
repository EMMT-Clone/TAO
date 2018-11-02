/*
 * macros.h --
 *
 * Definitions of useful macros for TAO.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#ifndef _TAO_MACROS_H_
#define _TAO_MACROS_H_ 1

#undef FALSE
#undef TRUE
#define FALSE  0
#define TRUE   1

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

/*
 * Helpers for branch prediction (See
 * http://blog.man7.org/2012/10/how-much-do-builtinexpect-likely-and.html).
 */
#define likely(expr)      __builtin_expect(!!(expr), 1)
#define unlikely(expr)    __builtin_expect(!!(expr), 0)

#endif /* _TAO_MACROS_H_ */
