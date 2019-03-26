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
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

#include "common.h"

#include <math.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_TIME_H
# include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include "tao-private.h"

#define if_likely(expr)   if TAO_LIKELY(expr)
#define if_unlikely(expr) if TAO_UNLIKELY(expr)

/*---------------------------------------------------------------------------*/
/* DYNAMIC MEMORY */

void*
tao_malloc(tao_error_t** errs, size_t size)
{
    void* ptr = malloc(size);
    if_unlikely(ptr == NULL) {
        tao_push_system_error(errs, "malloc");
    }
    return ptr;
}

void*
tao_calloc(tao_error_t** errs, size_t nelem, size_t elsize)
{
    void* ptr = calloc(nelem, elsize);
    if_unlikely(ptr == NULL) {
        tao_push_system_error(errs, "calloc");
    }
    return ptr;
}

void
tao_free(void* ptr)
{
    if_likely(ptr != NULL) {
        free(ptr);
    }
}

/*---------------------------------------------------------------------------*/
/* STRINGS */

size_t
tao_strlen(const char* str)
{
    return (TAO_UNLIKELY(str == NULL) ? 0 : strlen(str));
}

/*---------------------------------------------------------------------------*/
/* TIME */

/* Maximum value for `time_t`, assuming that it is a signed integer.  See
   https://stackoverflow.com/questions/5617925/maximum-values-for-time-t-struct-timespec */
#define TIME_T_MAX  TAO_MAX_SIGNED_INT(time_t)

#define KILO        1000
#define MEGA        1000000
#define GIGA        1000000000

#ifdef HAVE_CLOCK_GETTIME
# define GET_MONOTONIC_TIME(status, errs, dest)         \
    GET_TIME(status, errs, dest, CLOCK_MONOTONIC)
# define GET_CURRENT_TIME(status, errs, dest)           \
    GET_TIME(status, errs, dest, CLOCK_REALTIME)
# define GET_TIME(status, errs, dest, id)                       \
    do {                                                        \
        struct timespec t;                                      \
        if (clock_gettime(id, &t) == -1) {                      \
            tao_push_system_error(errs, "clock_gettime");       \
            dest->s = 0;                                        \
            dest->ns = 0;                                       \
            status = -1;                                        \
        } else {                                                \
            dest->s = t.tv_sec;                                 \
            dest->ns = t.tv_nsec;                               \
            status = 0;                                         \
        }                                                       \
    } while (false)
#else /* assume gettimeofday is available */
# define GET_MONOTONIC_TIME(status, errs, dest) \
    GET_CURRENT_TIME(status, errs, dest)
# define GET_CURRENT_TIME(status, errs, dest)                   \
    do {                                                        \
        struct timeval t;                                       \
        if (gettimeofday(&t, NULL) == -1) {                     \
            tao_push_system_error(errs, "gettimeofday");        \
            dest->s = 0;                                        \
            dest->ns = 0;                                       \
            status = -1;                                        \
        } else {                                                \
            dest->s = t.tv_sec;                                 \
            dest->ns = t.tv_usec*KILO;                          \
            status = 0;                                         \
        }                                                       \
    } while (false)
#endif

int
tao_get_monotonic_time(tao_error_t** errs, tao_time_t* dest)
{
    int status;
    GET_MONOTONIC_TIME(status, errs, dest);
    return status;
}

int
tao_get_current_time(tao_error_t** errs, tao_time_t* dest)
{
    int status;
    GET_CURRENT_TIME(status, errs, dest);
    return status;
}

#define NORMALIZE_TIME(s, ns)                   \
    do {                                        \
        (s) += (ns)/GIGA;                       \
        (ns) = (ns)%GIGA;                       \
        if ((ns) < 0) {                         \
            (s) -= 1;                           \
            (ns) += GIGA;                       \
        }                                       \
    } while (false)

tao_time_t*
tao_normalize_time(tao_time_t* ts)
{
    int64_t s  = ts->s;
    int64_t ns = ts->ns;
    NORMALIZE_TIME(s, ns);
    ts->s = s;
    ts->ns = ns;
    return ts;
}

tao_time_t*
tao_add_times(tao_time_t* dest, const tao_time_t* a, const tao_time_t* b)
{
    int64_t s  = a->s  + b->s;
    int64_t ns = a->ns + b->ns;
    NORMALIZE_TIME(s, ns);
    dest->s = s;
    dest->ns = ns;
    return dest;
}

tao_time_t*
tao_subtract_times(tao_time_t* dest, const tao_time_t* a, const tao_time_t* b)
{
    int64_t s  = a->s  - b->s;
    int64_t ns = a->ns - b->ns;
    NORMALIZE_TIME(s, ns);
    dest->s = s;
    dest->ns = ns;
    return dest;
}

double
tao_time_to_seconds(const tao_time_t* t)
{
    return (double)t->s + 1E-9*(double)t->ns;
}

tao_time_t*
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
    return dest;
}

char*
tao_sprintf_time(char* str, const tao_time_t* ts)
{
    int64_t s  = ts->s;
    int64_t ns = ts->ns;
    NORMALIZE_TIME(s, ns);
    bool negate = (s < 0);
    if (negate) {
        s = -s;
        ns = -ns;
        NORMALIZE_TIME(s, ns);
    }
    sprintf(str, "%s%ld.%09d", (negate ? "-" : ""), (long)s, (int)ns);
    return str;
}

size_t
tao_snprintf_time(char* str, size_t size, const tao_time_t* ts)
{
    char buf[32];
    tao_sprintf_time(buf, ts);
    size_t len = strlen(buf);
    if (str != NULL && size > 0) {
        if (len < size) {
            strcpy(str, buf);
        } else if (size > 1) {
            strncpy(str, buf, size - 1);
            str[size - 1] = '\0';
        } else {
            str[0] = '\0';
        }
    }
    return len;
}

void
tao_fprintf_time(FILE *stream, const tao_time_t* ts)
{
    char buf[32];
    tao_sprintf_time(buf, ts);
    fputs(buf, stream);
}

int
tao_get_absolute_timeout(tao_error_t** errs, tao_time_t* tm, double secs)
{
    if_unlikely(isnan(secs) || secs < 0) {
        tao_push_error(errs, __func__, TAO_BAD_ARGUMENT);
        return -1;
    }

    /* Get current time as soon as possible, so that the time consumed by the
       following (expensive) computations is automatically taken cinto
       account. */
    int status;
    GET_CURRENT_TIME(status, errs, tm);
    if_unlikely(status != 0) {
        return -1;
    }

    /* First just add the number of nanoseconds and normalise the result, then
       check for `time_t` overflow.  We are assuming that current time since
       the Epoch is not near the limit TIME_T_MAX. */
    double incr_s = floor(secs);
    long tm_s  = tm->s;
    long tm_ns = tm->ns + lround((secs - incr_s)*1e9);
    NORMALIZE_TIME(tm_s, tm_ns);
    if (tm_s + incr_s > (double)TIME_T_MAX) {
        tm->s = TIME_T_MAX;
        tm->ns = GIGA - 1;
    } else {
        tm->s  = tm_s + (long)incr_s;
        tm->ns = tm_ns;
    }
    return 0;
}

bool
tao_is_finite_absolute_time(tao_time_t* tm)
{
    return (tm->s < TIME_T_MAX || tm->ns < GIGA - 1);
}

double
tao_get_maximum_absolute_time()
{
    return (double)TIME_T_MAX;
}
