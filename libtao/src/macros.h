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

#define FALSE  0
#define TRUE  (!FALSE)

/*
 * Helpers for branch prediction (See
 * http://blog.man7.org/2012/10/how-much-do-builtinexpect-likely-and.html).
 */
#define likely(expr)      __builtin_expect(!!(expr), 1)
#define unlikely(expr)    __builtin_expect(!!(expr), 0)

#endif /* _TAO_MACROS_H_ */
