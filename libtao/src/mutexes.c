/*
 * mutexes.c --
 *
 * Management of mutexes and condition variables in TAO library.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

#include "common.h"

#include <errno.h>
#include <math.h>

#include "tao-private.h"

#define if_likely(expr)   if TAO_LIKELY(expr)
#define if_unlikely(expr) if TAO_UNLIKELY(expr)

/*
 * Notes:
 *
 * a. In Linux POSIX Threads doc., it is written that pthread_cond_init(),
 *    pthread_cond_signal(), pthread_cond_broadcast(), and pthread_cond_wait()
 *    never return an error.  However, in the Open Group Specifications (Issue
 *    7, 2018 edition IEEE Std 1003.1-2017, Revision of IEEE Std 1003.1-2008),
 *    it is written that, if successful, these functions shall return zero;
 *    otherwise, an error number shall be returned to indicate the error.
 */

/* See https://stackoverflow.com/questions/20325146 for configuring mutexes
   and condition variables shared between processes. */
int
tao_initialize_mutex(tao_error_t** errs, pthread_mutex_t* mutex, int shared)
{
    pthread_mutexattr_t attr;
    int code;

    /* Initialise attribute of mutex. */
    code = pthread_mutexattr_init(&attr);
    if_unlikely(code != 0) {
        /* This should never occur, but... */
        tao_push_error(errs, "pthread_mutexattr_init", code);
        return -1;
    }
    code = pthread_mutexattr_setpshared(&attr, (shared ?
                                                PTHREAD_PROCESS_SHARED :
                                                PTHREAD_PROCESS_PRIVATE));
    if_unlikely(code != 0) {
        pthread_mutexattr_destroy(&attr);
        tao_push_error(errs, "pthread_mutexattr_setpshared", code);
        return -1;
    }

    /* Initialise mutex. */
    code = pthread_mutex_init(mutex, &attr);
    if_unlikely(code != 0) {
        pthread_mutexattr_destroy(&attr);
        tao_push_error(errs, "pthread_mutex_init", code);
        return -1;
    }

    /* Destroy mutex attributes. */
    code = pthread_mutexattr_destroy(&attr);
    if_unlikely(code != 0) {
        /* This should never occur, but... */
        tao_push_error(errs, "pthread_mutexattr_destroy", code);
        return -1;
    }

    /* Report success. */
    return 0;
}

int
tao_lock_mutex(tao_error_t** errs, pthread_mutex_t* mutex)
{
    int code = pthread_mutex_lock(mutex);
    if_unlikely(code != 0) {
        tao_push_error(errs, "pthread_mutex_lock", code);
        return -1;
    }
    return 0;
}

int
tao_try_lock_mutex(tao_error_t** errs, pthread_mutex_t* mutex)
{
    int code = pthread_mutex_trylock(mutex);
    if (code == 0) {
        return 1;
    }
    if (code == EBUSY) {
        return 0;
    }
    tao_push_error(errs, "pthread_mutex_trylock", code);
    return -1;
}

int
tao_unlock_mutex(tao_error_t** errs, pthread_mutex_t* mutex)
{
    int code = pthread_mutex_unlock(mutex);
    if_unlikely(code != 0) {
        tao_push_error(errs, "pthread_mutex_unlock", code);
        return -1;
    }
    return 0;
}

/*
 * Attempt to destroy the mutex, possibly blocking until the mutex is
 * unlocked (signaled by pthread_mutex_destroy returning EBUSY).
 */
int
tao_destroy_mutex(tao_error_t** errs, pthread_mutex_t* mutex, bool wait)
{
    /*
     * Attempt to destroy the mutex, possibly blocking until the mutex is
     * unlocked (signaled by a pthread_mutex_destroy returning EBUSY).  In
     * order to not consume CPU, if the mutex was locked, we wait to become the
     * owner of the lock before re-trying to destroy the mutex.
     */
    while (true) {
        /* Attempt to destroy the mutex. */
        int code = pthread_mutex_destroy(mutex);
        if (code == 0) {
            /* Operation was successful. */
            return 0;
        }
        if (!wait || code != EBUSY) {
            tao_push_error(errs, "pthread_mutex_destroy", code);
            return -1;
        }

        /* The mutex is currently locked.  Wait for the owner of the lock
           to unlock before re-trying to destroy the mutex. */
        if (tao_lock_mutex(errs, mutex) != 0 ||
            tao_unlock_mutex(errs, mutex) != 0) {
            return -1;
        }
    }
}

int
tao_signal_condition(tao_error_t** errs, pthread_cond_t* cond)
{
    int code = pthread_cond_signal(cond);
    if_unlikely(code != 0) {
        /* This should never happen on Linux (see Note a.). */
        tao_push_error(errs, "pthread_cond_signal", code);
        return -1;
    }
    return 0;
}

int
tao_broadcast_condition(tao_error_t** errs, pthread_cond_t* cond)
{
    int code = pthread_cond_broadcast(cond);
    if_unlikely(code != 0) {
        /* This should never happen on Linux (see Note a.). */
        tao_push_error(errs, "pthread_cond_broadcast", code);
        return -1;
    }
    return 0;
}

int
tao_wait_condition(tao_error_t** errs, pthread_cond_t* cond,
                   pthread_mutex_t* mutex)
{
    int code = pthread_cond_wait(cond, mutex);
    if_unlikely(code != 0) {
        /* This should never happen on Linux (see Note a.). */
        tao_push_error(errs, "pthread_cond_wait", code);
        return -1;
    }
    return 0;
}

int
tao_timed_wait_condition(tao_error_t** errs, pthread_cond_t* cond,
                         pthread_mutex_t* mutex, double secs)
{
    int code;

    if_unlikely(isnan(secs) || secs < 0) {
        tao_push_error(errs, __func__, TAO_BAD_ARGUMENT);
        return -1;
    }
    if (secs > TAO_YEAR) {
        /* For a very long timeout, we just call `pthread_cond_wait`. */
    forever:
        code = pthread_cond_wait(cond, mutex);
        if_unlikely(code != 0) {
            /* This should never happen on Linux (see Note a.). */
            tao_push_error(errs, "pthread_cond_wait", code);
            return -1;
        }
    } else {
        struct timespec ts;
        if (tao_get_absolute_timeout(errs, &ts, secs) != 0) {
            return -1;
        }
        if (! tao_is_finite_absolute_time(&ts)) {
            goto forever;
        }
        code = pthread_cond_timedwait(cond, mutex, &ts);
        if (code != 0) {
            if (code == ETIMEDOUT) {
                return 0;
            }
            tao_push_error(errs, "pthread_cond_timedwait", code);
            return -1;
        }
    }
    return 1;
}
