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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "andor.h"
#include "andor-features.h"

static int
check0(tao_error_t** errs, const char* func, int code)
{
    if (code != AT_SUCCESS) {
        _andor_push_error(errs, func, code);
    }
    return code;
}

#if 0
static int
check1(tao_error_t** errs, const char* func, int code)
{
    if (code != AT_SUCCESS) {
        _andor_push_error(errs, func, code);
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
#define SDK_VERSION_MAXLEN 32
static char sdk_version[SDK_VERSION_MAXLEN+1];

long
andor_get_ndevices(tao_error_t** errs)
{
    if (ndevices < 0 && andor_initialize(errs) != 0) {
        return -1L;
    }
    return ndevices;
}

const char*
andor_get_software_version(tao_error_t** errs)
{
    if (sdk_version[0] == 0) {
        AT_WC version[SDK_VERSION_MAXLEN+1];
        tao_error_t* errs = TAO_NO_ERRORS;
        int status;
        if (ndevices < 0) {
            if (andor_initialize(&errs) != 0) {
            error:
                tao_report_errors(&errs);
                strcat(sdk_version, "0.0.0");
                return sdk_version;
            }
        }
        status = AT_GetString(AT_HANDLE_SYSTEM,
                              andor_feature_names[SoftwareVersion],
                              version, SDK_VERSION_MAXLEN);
        if (status != AT_SUCCESS) {
            _andor_push_error(&errs, "AT_GetString(SoftwareVersion)", status);
            goto error;
        }
        tao_discard_errors(&errs); /* not needed in principle */
        for (int i = 0; i < SDK_VERSION_MAXLEN && version[i] != 0; ++i) {
            sdk_version[i] = (char)version[i];
        }
        sdk_version[SDK_VERSION_MAXLEN] = 0;
    }
    return sdk_version;
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
    cam->state = 0;
    status = AT_Open(dev, &cam->handle);
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_Open", status);
        goto error;
    }
    if (andor_update_configuration(cam, true) != 0) {
        goto error;
    }
    cam->state = 1;
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

static void
free_buffers(andor_camera_t* cam)
{
    void** bufs = cam->bufs;
    long nbufs = cam->nbufs;
    cam->bufs = NULL;
    cam->nbufs = 0;
    cam->bufsiz = 0;
    if (bufs != NULL) {
        for (long k = 0; k < nbufs; ++k) {
            void* buf = bufs[k];
            if (buf != NULL) {
                free(buf);
            }
        }
        free(bufs);
    }
}

void
andor_close_camera(andor_camera_t* cam)
{
    if (cam != NULL) {
        tao_discard_errors(&cam->errs);
        if (cam->handle != AT_HANDLE_SYSTEM) {
            (void)AT_Flush(cam->handle);
            (void)AT_Close(cam->handle);
        }
        free_buffers(cam);
        free((void*)cam);
    }
}

int
andor_update_configuration(andor_camera_t* cam, bool all)
{
    int status;
    AT_64 ival;
    double fval;

    if (all) {
        /* First get the sensor size. */
        status = AT_GetInt(cam->handle, L"SensorWidth", &ival);
        if (status == AT_SUCCESS) {
            cam->sensorwidth = ival;
        } else {
            andor_push_error(cam, "AT_GetInt(Sensorwidth)", status);
            return -1;
        }
        status = AT_GetInt(cam->handle, L"SensorHeight", &ival);
        if (status == AT_SUCCESS) {
            cam->sensorheight = ival;
        } else {
            andor_push_error(cam, "AT_GetInt(Sensorheight)", status);
            return -1;
        }

        status = AT_GetInt(cam->handle, L"AOIHBin", &ival);
        if (status == AT_SUCCESS) {
            cam->config.xbin = ival;
        } else if (status == AT_ERR_NOTIMPLEMENTED) {
            cam->config.xbin = 1;
        } else {
            andor_push_error(cam, "AT_GetInt(AOIHBin)", status);
            return -1;
        }

        status = AT_GetInt(cam->handle, L"AOIVBin", &ival);
        if (status == AT_SUCCESS) {
            cam->config.ybin = ival;
        } else if (status == AT_ERR_NOTIMPLEMENTED) {
            cam->config.ybin = 1;
        } else {
            andor_push_error(cam, "AT_GetInt(AOIVBin)", status);
            return -1;
        }

        status = AT_GetInt(cam->handle, L"AOILeft", &ival);
        if (status == AT_SUCCESS) {
            cam->config.xoff = ival - 1;
        } else if (status == AT_ERR_NOTIMPLEMENTED) {
            cam->config.xbin = 0;
        } else {
            andor_push_error(cam, "AT_GetInt(AOILeft)", status);
            return -1;
        }

        status = AT_GetInt(cam->handle, L"AOITop", &ival);
        if (status == AT_SUCCESS) {
            cam->config.yoff = ival - 1;
        } else if (status == AT_ERR_NOTIMPLEMENTED) {
            cam->config.ybin = 0;
        } else {
            andor_push_error(cam, "AT_GetInt(AOITop)", status);
            return -1;
        }

        status = AT_GetInt(cam->handle, L"AOIWidth", &ival);
        if (status == AT_SUCCESS) {
            cam->config.width = ival;
        } else if (status == AT_ERR_NOTIMPLEMENTED) {
            cam->config.width = cam->sensorwidth;
        } else {
            andor_push_error(cam, "AT_GetInt(AOIWidth)", status);
            return -1;
        }

        status = AT_GetInt(cam->handle, L"AOIHeight", &ival);
        if (status == AT_SUCCESS) {
            cam->config.height = ival;
        } else if (status == AT_ERR_NOTIMPLEMENTED) {
            cam->config.height = cam->sensorheight;
        } else {
            andor_push_error(cam, "AT_GetInt(AOIHeight)", status);
            return -1;
        }

#if 0
        status = AT_GetInt(cam->handle, L"AOIStride", &ival);
        if (status == AT_SUCCESS) {
            cam->config.stride = ival;
        } else {
            andor_push_error(cam, "AT_GetInt(AOIStride)", status);
            return -1;
        }
#endif

#if 0
        status = AT_GetInt(cam->handle, L"ImageSizeBytes", &ival);
        if (status == AT_SUCCESS) {
            cam->config.buffersize = ival;
        } else {
            andor_push_error(cam, "AT_GetInt(ImageSizeBytes)", status);
            return -1;
        }
#endif

        /* According to the doc., Apogee has ne ExposureTime feature. */
        status = AT_GetFloat(cam->handle, L"ExposureTime", &fval);
        if (status == AT_SUCCESS) {
            cam->config.exposuretime = fval;
        } else {
            andor_push_error(cam, "AT_GetFloat(ExposureTime)", status);
            return -1;
        }

        status = AT_GetFloat(cam->handle, L"FrameRate", &fval);
        if (status == AT_SUCCESS) {
            cam->config.framerate = fval;
        } else {
            andor_push_error(cam, "AT_GetFloat(FrameRate)", status);
            return -1;
        }
    }

    status = AT_GetFloat(cam->handle, L"SensorTemperature", &fval);
    if (status == AT_SUCCESS) {
        cam->config.temperature = fval;
    } else {
        andor_push_error(cam, "AT_GetFloat(SensorTemperature)", status);
        return -1;
    }

    return 0;
}
