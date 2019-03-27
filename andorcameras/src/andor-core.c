/*
 * andor-core.c --
 *
 * Core functions for Andor cameras.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "andorcameras.h"
#include "andor-features.h"

static int
check0(tao_error_t** errs, const char* func, int code)
{
    if (code != AT_SUCCESS) {
        andor_push_error(errs, func, code);
    }
    return code;
}

#if 0
static int
check1(tao_error_t** errs, const char* func, int code)
{
    if (code != AT_SUCCESS) {
        andor_push_error(errs, func, code);
        return -1;
    } else {
        return 0;
    }
}
#endif

#define CALL0(errs, func, args) check0(errs, #func, func args)
#define CALL1(errs, func, args) check1(errs, #func, func args)

/* FIXME: shall we protect this by a mutex? */
static long ndevices = -2;

long
andor_get_ndevices(tao_error_t** errs)
{
    if (ndevices < 0 && andor_initialize(errs) != 0) {
        return -1L;
    }
    return ndevices;
}

#if 0
int
andor_initialize_library(tao_error_t** errs)
{
    return CALL1(errs, AT_InitialiseLibrary, ());
}
#endif

#if 0
int
andor_finalize_library(tao_error_t** errs)
{
    return CALL1(errs, AT_FinaliseLibrary, ());
}
#endif

static const char* logfile = "/tmp/atdebug.log";
static mode_t logmode = (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

static void finalize(void)
{
    /* Finalize the library (if it has been initialized) and try to change
       access permissions for the logfile to avoid crash for others.  Any
       errors are ignoored here. */
    if (ndevices >= 0) {
        ndevices = -1;
        (void)AT_InitialiseLibrary();
    }
    (void)chmod(logfile, logmode);
}

int
andor_initialize(tao_error_t** errs)
{
    AT_64 ival;
    int status;

    if (ndevices == -2) {
        /* Before initializing Andor Core library, we set r/w permissions for
           all users to the log file "/tmp/atdebug.log" (otherwise
           initialization will crash for others). */
        int fd = open(logfile, O_RDWR|O_CREAT, logmode);
        if (fd == -1) {
            fprintf(stderr,
                    "ERROR: Log file \"%s\" cannot be open/created for "
                    "r/w access.\n       You may either remove this file or "
                    "change its access permissions with:\n       "
                    "$ sudo chmod a+rw %s\n", logfile, logfile);
            tao_push_system_error(errs, "open");
            return -1;
        }
        (void)close(fd);

        /* Initialize Andor Core library and schedule to perform finalization
           at exit. */
        status = CALL0(errs, AT_InitialiseLibrary,());
        if (status != AT_SUCCESS) {
            return -1;
        }
        if (atexit(finalize) != 0) {
            tao_push_system_error(errs, "atexit");
            return -1;
        }
        ndevices = -1;
    }
    if (ndevices == -1) {
        status = CALL0(errs,
                       AT_GetInt,(AT_HANDLE_SYSTEM, L"DeviceCount", &ival));
        if (status != AT_SUCCESS) {
            return -1;
        }
        if (ival < 0) {
            tao_push_error(errs, __func__, TAO_ASSERTION_FAILED);
            return -1;
        }
        ndevices = ival;
    }
    return 0;
}

andor_camera_t*
andor_open_camera(tao_error_t** errs, long dev)
{
    andor_camera_t* cam;
    int status;
    long ndevices;

    ndevices = andor_get_ndevices(errs);
    if (ndevices < 0) {
        return NULL;
    }
    if (dev < 0 || dev >= ndevices) {
        tao_push_error(errs, __func__, TAO_BAD_DEVICE);
        return NULL;
    }
    cam = (andor_camera_t*)tao_calloc(errs, 1, sizeof(andor_camera_t));
    if (cam == NULL) {
        return NULL;
    }
    cam->handle = AT_HANDLE_SYSTEM;
    cam->errs = TAO_NO_ERRORS;
    status = AT_Open(dev, &cam->handle);
    if (status != AT_SUCCESS) {
        andor_push_error(errs, "AT_Open", status);
        goto error;
    }
    if (ANDOR_GET_INTEGER(cam, SensorWidth,  &cam->sensorwidth) != 0 ||
        ANDOR_GET_INTEGER(cam, SensorHeight, &cam->sensorheight) != 0) {
        goto error;
    }
    return cam;

    /* We branch here in case of errors. */
 error:
    if (cam != NULL) {
        tao_transfer_errors(errs, &cam->errs);
        cam->handle = AT_HANDLE_SYSTEM;
        andor_close_camera(cam);
    }
    return NULL;
}

void
andor_close_camera(andor_camera_t* cam)
{
    if (cam != NULL) {
        tao_discard_errors(&cam->errs);
        if (cam->handle != AT_HANDLE_SYSTEM) {
            (void)AT_Close(cam->handle);
        }
        free((void*)cam);
    }
}
