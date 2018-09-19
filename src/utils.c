/*
 * utils.c --
 *
 * Utility functions.  Mostly provided to track errors.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <stdlib.h>

#include "tao-private.h"

void*
tao_malloc(tao_error_t** errs, size_t size)
{
    void* ptr = malloc(size);
    if (ptr == NULL) {
        tao_push_system_error(errs, "malloc");
    }
    return ptr;
}

void*
tao_calloc(tao_error_t** errs, size_t nelem, size_t elsize)
{
    void* ptr = calloc(nelem, elsize);
    if (ptr == NULL) {
        tao_push_system_error(errs, "calloc");
    }
    return ptr;
}

void
tao_free(void* ptr)
{
    if (ptr != NULL) {
        free(ptr);
    }
}
