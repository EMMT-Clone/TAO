/*
 * mutexes.c --
 *
 * Management of mutexes in TAO library.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <errno.h>

#include "config.h"
#include "tao-private.h"

/* See https://stackoverflow.com/questions/20325146 for configuring mutexes
   and condition variables shared between processes. */
int
tao_initialize_mutex(tao_error_t** errs, pthread_mutex_t* mutex, int shared)
{
    pthread_mutexattr_t attr;
    int code;

    /* Initialise attribute of mutex. */
    code = pthread_mutexattr_init(&attr);
    if (code != 0) {
        /* This should never occur, but... */
        tao_push_error(errs, "pthread_mutexattr_init", code);
        return -1;
    }
    code = pthread_mutexattr_setpshared(&attr, (shared ?
                                                PTHREAD_PROCESS_SHARED :
                                                PTHREAD_PROCESS_PRIVATE));
    if (code != 0) {
        pthread_mutexattr_destroy(&attr);
        tao_push_error(errs, "pthread_mutexattr_setpshared", code);
        return -1;
    }

    /* Initialise mutex. */
    code = pthread_mutex_init(mutex, &attr);
    if (code != 0) {
        pthread_mutexattr_destroy(&attr);
        tao_push_error(errs, "pthread_mutex_init", code);
        return -1;
    }

    /* Destroy mutex attributes. */
    code = pthread_mutexattr_destroy(&attr);
    if (code != 0) {
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
    if (code != 0) {
        tao_push_error(errs, "pthread_mutex_lock", code);
        return -1;
    } else {
        return 0;
    }
}

int
tao_try_lock_mutex(tao_error_t** errs, pthread_mutex_t* mutex)
{
    int code = pthread_mutex_trylock(mutex);
    if (code == 0) {
        return 1;
    } else if (code == EBUSY) {
        return 0;
    } else {
        tao_push_error(errs, "pthread_mutex_trylock", code);
        return -1;
    }
}

int
tao_unlock_mutex(tao_error_t** errs, pthread_mutex_t* mutex)
{
    int code = pthread_mutex_unlock(mutex);
    if (code != 0) {
        tao_push_error(errs, "pthread_mutex_unlock", code);
        return -1;
    } else {
        return 0;
    }
}

int
tao_destroy_mutex(tao_error_t** errs, pthread_mutex_t* mutex)
{
    while (1) {
        /* Attempt to destroy the mutex. */
        int code = pthread_mutex_destroy(mutex);
        if (code == 0) {
            /* Operation was successful. */
            return 0;
        }
        if (code != EBUSY) {
            tao_push_error(errs, "pthread_mutex_destroy", code);
            return -1;
        }

        /* The mutex is currently locked.  Wait for the owner of the lock
           to unlock before re-trying to destroy the mutex. */
        code = pthread_mutex_lock(mutex);
        if (code != 0) {
            tao_push_error(errs, "pthread_mutex_lock", code);
            return -1;
         }
        code = pthread_mutex_unlock(mutex);
        if (code != 0) {
            tao_push_error(errs, "pthread_mutex_unlock", code);
            return -1;
         }
    }
}

int
tao_signal_condition(tao_error_t** errs, pthread_cond_t* cond)
{
    int code = pthread_cond_signal(cond);
    if (code != 0) {
        tao_push_error(errs, "pthread_cond_signal", code);
        return -1;
    }
    return 0;
}
