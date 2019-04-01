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
#include <errno.h>
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


int
tao_parse_int(const char* str, int* ptr, int base)
{
    long val;
    if (ptr == NULL) {
        errno = EFAULT;
        return -1;
    }
    if (tao_parse_long(str, &val, base) != 0) {
        return -1;
    }
    if (val < INT_MIN || val > INT_MAX) {
        errno = ERANGE;
        return -1;
    }
    *ptr = (int)val;
    return 0;
}

int
tao_parse_long(const char* str, long* ptr, int base)
{
    char* end;
    long val;
    if (str == NULL || ptr == NULL) {
        errno = EFAULT;
        return -1;
    }
    errno = 0; /* to detect overflows */
    val = strtol(str, &end, base);
    if (end == str || *end != '\0' || errno != 0) {
        return -1;
    }
    *ptr = val; /* only change value in case of success */
    return 0;
}

int
tao_parse_double(const char* str, double* ptr)
{
    char* end;
    double val;
    if (str == NULL || ptr == NULL) {
        errno = EFAULT;
        return -1;
    }
    val = strtod(str, &end);
    if (end == str || *end != '\0') {
        return -1;
    }
    *ptr = val; /* only change value in case of success */
    return 0;
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
        status = clock_gettime(id, dest);                       \
        if_unlikely(status != 0) {                              \
            tao_push_system_error(errs, "clock_gettime");       \
            dest->tv_sec = 0;                                   \
            dest->tv_nsec = 0;                                  \
        }                                                       \
    } while (false)
#else /* assume gettimeofday is available */
# define GET_MONOTONIC_TIME(status, errs, dest) \
    GET_CURRENT_TIME(status, errs, dest)
# define GET_CURRENT_TIME(status, errs, dest)                   \
    do {                                                        \
        struct timeval tv;                                      \
        status = gettimeofday(&tv, NULL);                       \
        if_unlikely(status != 0) {                              \
            tao_push_system_error(errs, "gettimeofday");        \
            dest->tv_sec = 0;                                   \
            dest->tv_nsec = 0;                                  \
        } else {                                                \
            dest->tv_sec  = tv.tv_sec;                          \
            dest->tv_nsec = t.tv_usec*KILO;                     \
        }                                                       \
    } while (false)
#endif

int
tao_get_monotonic_time(tao_error_t** errs, struct timespec* dest)
{
    int status;
    GET_MONOTONIC_TIME(status, errs, dest);
    return status;
}

int
tao_get_current_time(tao_error_t** errs, struct timespec* dest)
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

struct timespec*
tao_normalize_time(struct timespec* ts)
{
    time_t s  = ts->tv_sec;
    time_t ns = ts->tv_nsec;
    NORMALIZE_TIME(s, ns);
    ts->tv_sec  = s;
    ts->tv_nsec = ns;
    return ts;
}

struct timespec*
tao_add_times(struct timespec* dest,
              const struct timespec* a, const struct timespec* b)
{
    time_t s  = a->tv_sec  + b->tv_sec;
    time_t ns = a->tv_nsec + b->tv_nsec;
    NORMALIZE_TIME(s, ns);
    dest->tv_sec = s;
    dest->tv_nsec = ns;
    return dest;
}

struct timespec*
tao_subtract_times(struct timespec* dest,
                   const struct timespec* a, const struct timespec* b)
{
    time_t s  = a->tv_sec  - b->tv_sec;
    time_t ns = a->tv_nsec - b->tv_nsec;
    NORMALIZE_TIME(s, ns);
    dest->tv_sec = s;
    dest->tv_nsec = ns;
    return dest;
}

double
tao_time_to_seconds(const struct timespec* t)
{
    return (double)t->tv_sec + 1E-9*(double)t->tv_nsec;
}

struct timespec*
tao_seconds_to_time(struct timespec* dest, double secs)
{
    /*
     * Take care of overflows (even though such overflows probably indicate an
     * invalid usage of the time).
     */
    if (isnan(secs)) {
        dest->tv_sec = 0;
        dest->tv_nsec = -1;
    } else if (secs >= INT64_MAX) {
        dest->tv_sec = INT64_MAX;
        dest->tv_nsec = 0;
    } else if (secs <= INT64_MIN) {
        dest->tv_sec = INT64_MIN;
        dest->tv_nsec = 0;
    } else {
        /*
         * Compute the number of seconds (as a floating-point) and the number
         * of nanoseconds (as an integer).  The formula guarantees that the
         * number of nanoseconds is nonnegative but it may be greater or equal
         * one billion so fix it.
         */
        double s = floor(secs);
        time_t ns = lround((secs - s)*1E9);
        if (ns >= GIGA) {
            ns -= GIGA;
            s += 1;
        }
        dest->tv_sec = (time_t)s;
        dest->tv_nsec = ns;
    }
    return dest;
}

char*
tao_sprintf_time(char* str, const struct timespec* ts)
{
    time_t s  = ts->tv_sec;
    time_t ns = ts->tv_nsec;
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
tao_snprintf_time(char* str, size_t size, const struct timespec* ts)
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
tao_fprintf_time(FILE *stream, const struct timespec* ts)
{
    char buf[32];
    tao_sprintf_time(buf, ts);
    fputs(buf, stream);
}

int
tao_get_absolute_timeout(tao_error_t** errs, struct timespec* tm, double secs)
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
    time_t tm_s  = tm->tv_sec;
    time_t tm_ns = tm->tv_nsec + lround((secs - incr_s)*1e9);
    NORMALIZE_TIME(tm_s, tm_ns);
    if (tm_s + incr_s > (double)TIME_T_MAX) {
        tm->tv_sec = TIME_T_MAX;
        tm->tv_nsec = GIGA - 1;
    } else {
        tm->tv_sec  = tm_s + (time_t)incr_s;
        tm->tv_nsec = tm_ns;
    }
    return 0;
}

bool
tao_is_finite_absolute_time(struct timespec* tm)
{
    return (tm->tv_sec < TIME_T_MAX || tm->tv_nsec < GIGA - 1);
}

double
tao_get_maximum_absolute_time()
{
    return (double)TIME_T_MAX;
}

int
tao_sleep(double secs)
{
    if_unlikely(isnan(secs) || secs < 0 || secs > TIME_T_MAX) {
        errno = EINVAL;
        return -1;
    }
    if (secs > 0) {
        struct timespec ts;
        time_t s = (time_t)secs;
        time_t ns = (time_t)((secs - (double)s)*1E9 + 0.5);
        NORMALIZE_TIME(s, ns);
        ts.tv_sec  = s;
        ts.tv_nsec = ns;
        return nanosleep(&ts, NULL);
    }
    return 0;
}
