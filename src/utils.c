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

#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#include "tao-private.h"

/*---------------------------------------------------------------------------*/
/* DYNAMIC MEMORY */

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

/*---------------------------------------------------------------------------*/
/* TIME */

#define GET_TIME(errs, dest, id)                                \
    do {                                                        \
        struct timespec t;                                      \
        if (clock_gettime(id, &t) == -1) {                      \
            tao_push_system_error(errs, "clock_gettime");       \
            dest->s = 0;                                        \
            dest->ns = -1;                                      \
            return -1;                                          \
        } else {                                                \
            dest->s = t.tv_sec;                                 \
            dest->ns = t.tv_nsec;                               \
            return 0;                                           \
        }                                                       \
    } while (0)

int
tao_get_monotonic_time(tao_error_t** errs, tao_time_t* dest)
{
    GET_TIME(errs, dest, CLOCK_MONOTONIC);
}

int
tao_get_current_time(tao_error_t** errs, tao_time_t* dest)
{
#if 1
    GET_TIME(errs, dest, CLOCK_REALTIME);
#else
    struct timeval t;
    if (gettimeofday(&t, NULL) == -1) {
        tao_push_system_error(errs, "gettimeofday");
        dest->s = 0;
        dest->ns = -1;
        return -1;
    } else {
        dest->s = t.tv_sec;
        dest->ns = t.tv_usec*1000;
        return 0;
    }
#endif
}

#define GIGA 1000000000

#define FIX_TIME(s, ns)                         \
    do {                                        \
        (s) += (ns)/GIGA;                       \
        (ns) = (ns)%GIGA;                       \
        if ((ns) < 0) {                         \
            (s) -= 1;                           \
            (ns) += GIGA;                       \
        }                                       \
    } while (0)

void
tao_add_times(tao_time_t* dest, const tao_time_t* a, const tao_time_t* b)
{
    int64_t s  = a->s  + b->s;
    int64_t ns = a->ns + b->ns;
    FIX_TIME(s, ns);
    dest->s = s;
    dest->ns = ns;
}

void
tao_subtract_times(tao_time_t* dest, const tao_time_t* a, const tao_time_t* b)
{
    int64_t s  = a->s  - b->s;
    int64_t ns = a->ns - b->ns;
    FIX_TIME(s, ns);
    dest->s = s;
    dest->ns = ns;
}

double
tao_time_to_seconds(const tao_time_t* t)
{
    return (double)t->s + 1E-9*(double)t->ns;
}

void
tao_seconds_to_time(tao_time_t* dest, double secs)
{
    /*
     * Take care of overflows (even though such overflows probably indicate an
     * invalid usage of the time).
     */
    if (isnan(secs)) {
        dest->s = 0;
        dest->ns = -1;
    } else if (secs >= INT64_MAX) {
        dest->s = INT64_MAX;
        dest->ns = 0;
    } else if (secs <= INT64_MIN) {
        dest->s = INT64_MIN;
        dest->ns = 0;
    } else {
        /*
         * Compute the number of seconds (as a floating-point) and the number
         * of nanoseconds (as an integer).  The formula guarantees that the
         * number of nanoseconds is nonnegative but it may be greater or equal
         * one billion so fix it.
         */
        double s = floor(secs);
        int64_t ns = lround((secs - s)*1E9);
        if (ns >= GIGA) {
            ns -= GIGA;
            s += 1;
        }
        dest->s = (int64_t)s;
        dest->ns = ns;
    }
}
