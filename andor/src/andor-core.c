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

#include <math.h>
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

    /* Get list of supported pixel encodings.  (Must be done before updating
       the configuration.) */
    cam->nencodings = andor_get_pixel_encodings(cam, cam->encodings,
                                                ANDOR_MAX_ENCODINGS);
    if (cam->nencodings < 0) {
        goto error;
    }

    /* Update other parameters from the hardware. */
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

const wchar_t *
andor_get_encoding_name(andor_pixel_encoding_t enc)
{
    switch (enc) {
    case ANDOR_ENCODING_MONO8: return L"Mono8";
    case ANDOR_ENCODING_MONO12: return L"Mono12";
    case ANDOR_ENCODING_MONO12CODED: return L"Mono12Coded";
    case ANDOR_ENCODING_MONO12CODEDPACKED: return L"Mono12CodedPacked";
    case ANDOR_ENCODING_MONO12PACKED: return L"Mono12Packed";
    case ANDOR_ENCODING_MONO16: return L"Mono16";
    case ANDOR_ENCODING_MONO22PACKEDPARALLEL: return L"Mono22PackedParallel";
    case ANDOR_ENCODING_MONO22PARALLEL: return L"Mono22Parallel";
    case ANDOR_ENCODING_MONO32: return L"Mono32";
    case ANDOR_ENCODING_RGB8PACKED: return L"RGB8Packed";
    default: return L"Unknown";
    }
}

andor_pixel_encoding_t
andor_get_encoding(const wchar_t* name)
{
    if (wcsncasecmp(name, L"Mono", 4) == 0) {
        switch (name[4]) {
        case '1':
            if (wcscasecmp(name, L"Mono12") == 0) {
                return ANDOR_ENCODING_MONO12;
            } else if (wcscasecmp(name, L"Mono12Coded") == 0) {
                return ANDOR_ENCODING_MONO12CODED;
            } else if (wcscasecmp(name, L"Mono12CodedPacked") == 0) {
                return ANDOR_ENCODING_MONO12CODEDPACKED;
            } else if (wcscasecmp(name, L"Mono12Packed") == 0) {
                return ANDOR_ENCODING_MONO12PACKED;
            } else if (wcscasecmp(name, L"Mono16") == 0) {
                return ANDOR_ENCODING_MONO16;
            }
            break;
        case '2':
            if (wcscasecmp(name, L"Mono22PackedParallel") == 0) {
                return ANDOR_ENCODING_MONO22PACKEDPARALLEL;
            } else if (wcscasecmp(name, L"Mono22Parallel") == 0) {
                return ANDOR_ENCODING_MONO22PARALLEL;
            }
            break;
        case '3':
            if (wcscasecmp(name, L"Mono32") == 0) {
                return ANDOR_ENCODING_MONO32;
            }
            break;
        case '8':
            if (wcscasecmp(name, L"Mono8") == 0) {
                return ANDOR_ENCODING_MONO8;
            }
            break;
        }
    } else if (wcscasecmp(name, L"RGB8Packed") == 0) {
        return ANDOR_ENCODING_RGB8PACKED;
    }
    return ANDOR_ENCODING_UNKNOWN;
}

long
andor_get_pixel_encodings(andor_camera_t* cam,
                          andor_pixel_encoding_t* encodings, long len)
{
    const int wstrlen = 32;
    AT_WC wstr[wstrlen];
    int cnt, status;

    /* Get the number of supported current pixel encoding. */
    status = AT_GetEnumCount(cam->handle, L"PixelEncoding", &cnt);
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_GetEnumCount(PixelEncoding)", status);
        return -1;
    }
    if (encodings == NULL) {
        return cnt;
    }
    if (len < cnt) {
        tao_push_error(&cam->errs, __func__, TAO_OUT_OF_RANGE);
        return -1;
    }
    for (int idx = 0; idx < cnt; ++idx) {
        status = AT_GetEnumStringByIndex(cam->handle, L"PixelEncoding",
                                         idx, wstr, wstrlen);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_GetEnumStringByIndex(PixelEncoding)",
                             status);
            return -1;
        }
        encodings[idx] = andor_get_encoding(wstr);
    }
    for (long idx = cnt; idx < len; ++idx) {
        encodings[idx] = -1;
    }
    return cnt;
}

int
andor_start(andor_camera_t* cam, long nbufs)
{
    long bufsiz;
    AT_64 ival;
    AT_BOOL bval;
    int idx, status;

    /* FIXME: lock some mutex? */

    /* Check camera state. */
    if (cam->state != 1) {
        andor_push_error(cam, __func__, TAO_ACQUISITION_RUNNING);
        return -1;
    }

    /* Make sure the camera does not use any old acquisition buffers. */
    status = AT_Flush(cam->handle);
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_Flush", status);
        return -1;
    }

    /* Get the current pixel encoding. */
    status = AT_GetEnumIndex(cam->handle, L"PixelEncoding", &idx);
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_GetInt(PixelEncoding)", status);
        return -1;
    }

    /* Get size of acquisition buffers in bytes. */
    status = AT_GetInt(cam->handle, L"ImageSizeBytes", &ival);
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_GetInt(ImageSizeBytes)", status);
        return -1;
    }
    bufsiz = ival;

    /* Get size of one row in the image in bytes. */
    status = AT_GetInt(cam->handle, L"AOIStride", &ival);
    if (status == AT_SUCCESS) {
        cam->stride = ival;
    } else {
        andor_push_error(cam, "AT_GetInt(AOIStride)", status);
        return -1;
    }

    /* Allocate new acquisition buffers if needed. */
    if (nbufs < 2) {
        nbufs = 2;
    }
    if (cam->nbufs != nbufs) {
        /* Reallocate everything. */
        free_buffers(cam);
        cam->bufs = (void**)tao_calloc(&cam->errs, nbufs, sizeof(void*));
        if (cam->bufs == NULL) {
            return -1;
        }
        cam->nbufs = nbufs;
    }
    if (cam->bufsiz != bufsiz) {
        /* Free all existing buffers. */
        for (long k = 0; k < nbufs; ++k) {
            void* buf = cam->bufs[k];
            cam->bufs[k] = NULL;
            if (buf != NULL) {
                free(buf);
            }
        }
        cam->bufsiz = bufsiz;
    }
    for (long k = 0; k < nbufs; ++k) {
        if (cam->bufs[k] == NULL) {
            cam->bufs[k] = tao_malloc(&cam->errs, bufsiz);
            if (cam->bufs[k] == NULL) {
                return -1;
            }
        }
    }

    /* Queue the acquisition buffers. */
    for (long k = 0; k < nbufs; ++k) {
        status = AT_QueueBuffer(cam->handle, (AT_U8*)cam->bufs[k], bufsiz);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_QueueBuffer", status);
            return -1;
        }
    }

    /* Set the camera to continuously acquires frames. */
    status = AT_IsImplemented(cam->handle, L"CycleMode", &bval);
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_IsImplemented(CycleMode)", status);
        return -1;
    }
    if (bval == AT_TRUE) {
        status = AT_SetEnumString(cam->handle, L"CycleMode", L"Continuous");
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetEnumString(CycleMode,Continuous)",
                             status);
            return -1;
        }
    }
    status = AT_IsImplemented(cam->handle, L"TriggerMode", &bval);
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_IsImplemented(TriggerMode)", status);
        return -1;
    }
    if (bval == AT_TRUE) {
        status = AT_SetEnumString(cam->handle, L"TriggerMode", L"Internal");
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetEnumString(TriggerMode,Internal)",
                             status);
            return -1;
        }
    }

    /* Start the acquisition. */
    status = AT_Command(cam->handle, L"AcquisitionStart");
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_Command(AcquisitionStart)", status);
        return -1;
    }

    /* Update state and return. */
    cam->state = 2;
    return 0;
}

int
andor_stop(andor_camera_t* cam)
{
    int status;

    /* FIXME: lock some mutex? */

    /* Check camera state. FIXME: use CameraAcquiring feature */
    if (cam->state != 2) {
        fprintf(stderr, "WARNING: Acquisition is not running\n");
        return 0;
    }

    /* Stop the acquisition. */
    status = AT_Command(cam->handle, L"AcquisitionStop");
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_Command(AcquisitionStop)", status);
        return -1;
    }

    /* Make sure the camera no longer use any acquisition buffers. */
    status = AT_Flush(cam->handle);
    if (status != AT_SUCCESS) {
        andor_push_error(cam, "AT_Flush", status);
        return -1;
    }

    /* Update state and return. */
    cam->state = 1;
    return 0;
}

int
andor_update_configuration(andor_camera_t* cam, bool all)
{
    int status, idx;
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

        /* Get current pixel encoding. */
        status = AT_GetEnumIndex(cam->handle, L"PixelEncoding", &idx);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_GetInt(PixelEncoding)", status);
            return -1;
        }
        if (idx < 0 || idx >= cam->nencodings) {
            tao_push_error(&cam->errs, __func__, TAO_OUT_OF_RANGE);
            return -1;
        }
        cam->config.pixelencoding = cam->encodings[idx];
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

void
andor_get_configuration(andor_camera_t* cam, andor_camera_config_t* cfg)
{
    memcpy(cfg, &cam->config, sizeof(andor_camera_config_t));
}

int
andor_set_configuration(andor_camera_t* cam, const andor_camera_config_t* cfg)
{
    int status, pixelencoding_index;
    bool change_roi, changes;

    /* Check configuration before applying any changes. */
    change_roi = (cfg->xoff != cam->config.xoff ||
                  cfg->yoff != cam->config.yoff ||
                  cfg->width != cam->config.width ||
                  cfg->height != cam->config.height ||
                  cfg->xbin != cam->config.xbin ||
                  cfg->ybin != cam->config.ybin);
    if (change_roi) {
        changes = true;
        if (cfg->xbin < 1 || cfg->ybin < 1) {
            tao_push_error(&cam->errs, __func__, TAO_BAD_SIZE);
            return -1;
        }
        if (cfg->xoff < 1 || cfg->width < 1 ||
            cfg->xoff + cfg->width*cfg->xbin > cam->sensorwidth ||
            cfg->yoff < 1 || cfg->height < 1 ||
            cfg->yoff + cfg->height*cfg->ybin > cam->sensorheight) {
            tao_push_error(&cam->errs, __func__, TAO_BAD_ROI);
            return -1;
        }
    }
    if (cfg->exposuretime != cam->config.exposuretime) {
        changes = true;
        if (isnan(cfg->exposuretime) || isinf(cfg->exposuretime) ||
            cfg->exposuretime < 0) {
            tao_push_error(&cam->errs, __func__, TAO_BAD_EXPOSURE);
            return -1;
        }
    }
    if (cfg->framerate != cam->config.framerate) {
        changes = true;
        if (isnan(cfg->framerate) || isinf(cfg->framerate) ||
            cfg->framerate <= 0) {
            tao_push_error(&cam->errs, __func__, TAO_BAD_EXPOSURE);
            return -1;
        }
    }
    pixelencoding_index = -1;
    if (cfg->pixelencoding != cam->config.pixelencoding) {
        changes = true;
        if (cfg->pixelencoding >= ANDOR_ENCODING_MIN &&
            cfg->pixelencoding <= ANDOR_ENCODING_MAX) {
            for (long k; k < cam->nencodings; ++k) {
                if (cam->encodings[k] == cfg->pixelencoding) {
                    pixelencoding_index = k;
                    break;
                }
            }
        }
        if (pixelencoding_index == -1) {
            tao_push_error(&cam->errs, __func__, TAO_BAD_ENCODING);
            return -1;
        }
    }

    /* Change pixel encoding if requested so. */
    if (pixelencoding_index != -1) {
        status = AT_SetEnumIndex(cam->handle, L"PixelEncoding",
                                 pixelencoding_index);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetInt(PixelEncoding)", status);
            return -1;
        }
        cam->config.pixelencoding = cam->config.pixelencoding;
     }

    if (change_roi) {
        /* Change the ROI parameters in the order recommended in the Andor SDK
           doc.  We are conservative here: since chaging one parameter may
           impact another one, with apply all settings in order even though
           they may be identical to the current configuration.  */
        status = AT_SetInt(cam->handle, L"AOIHBin", cfg->xbin);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetInt(AOIHBin)", status);
            return -1;
        }
        cam->config.xbin = cfg->xbin;
        status = AT_SetInt(cam->handle, L"AOIVBin", cfg->ybin);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetInt(AOIVBin)", status);
            return -1;
        }
        cam->config.ybin = cfg->ybin;
        status = AT_SetInt(cam->handle, L"AOIWidth", cfg->width);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetInt(AOIWidth)", status);
            return -1;
        }
        cam->config.width = cfg->width;
        status = AT_SetInt(cam->handle, L"AOILeft", cfg->xoff + 1);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetInt(AOILeft)", status);
            return -1;
        }
        cam->config.xoff = cfg->xoff;
        status = AT_SetInt(cam->handle, L"AOIHeight", cfg->height);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetInt(AOIHeight)", status);
            return -1;
        }
        cam->config.height = cfg->height;
        status = AT_SetBool(cam->handle, L"VerticallyCentreAOI", AT_FALSE);
        if (status != AT_SUCCESS && status != AT_ERR_NOTIMPLEMENTED) {
            andor_push_error(cam, "AT_SetInt(VerticallyCentreAOI)", status);
            return -1;
        }
        status = AT_SetInt(cam->handle, L"AOITop", cfg->yoff + 1);
        if (status != AT_SUCCESS) {
            andor_push_error(cam, "AT_SetInt(AOITop)", status);
            return -1;
        }
        cam->config.yoff = cfg->yoff;
    }

    /* Change frame rate and exposure time.  First reduce the frame rate if
       requested, then change the exposure time if requested, finally augment
       the frame rate if requested. */
    for (int pass = 1; pass <= 2; ++pass) {
        if (pass == 1
            ? cfg->framerate < cam->config.framerate
            : cfg->framerate > cam->config.framerate) {
            status = AT_SetFloat(cam->handle, L"FrameRate", cfg->framerate);
            if (status != AT_SUCCESS) {
                andor_push_error(cam, "AT_SetFloat(FrameRate)", status);
                return -1;
            }
            cam->config.framerate = cfg->framerate;
        }
        if (pass == 2) {
            break;
        }
        if (cfg->exposuretime != cam->config.exposuretime) {
            status = AT_SetFloat(cam->handle, L"ExposureTime",
                                 cfg->exposuretime );
            if (status != AT_SUCCESS) {
                andor_push_error(cam, "AT_SetFloat(ExposureTime)", status);
                return -1;
            }
            cam->config.exposuretime = cfg->exposuretime;
        }
    }

    /* If anything has changed, update the configuration in case of. */
    if (changes && andor_update_configuration(cam, true) != 0) {
        return -1;
    }

    return 0;
}

void
andor_print_configuration(FILE* output, const andor_camera_config_t* cfg)
{
    fprintf(output, "Sensor temperature: %.1f°C\n", cfg->temperature);
    fprintf(output, "Pixel binning: %ld×%ld\n",
            cfg->xbin, cfg->ybin);
    fprintf(output, "Region of interest: %ld×%ld at (%ld,%ld)\n",
            cfg->width, cfg->height,
            cfg->xoff, cfg->yoff);
    fprintf(output, "Exposure time: %g s\n", cfg->exposuretime);
    fprintf(output, "Frame rate: %g Hz\n", cfg->framerate);
    fprintf(output, "Pixel encoding: %ls\n",
            andor_get_encoding_name(cfg->pixelencoding));
}

void
andor_print_camera_configuration(FILE* output, const andor_camera_t* cam)
{
    fprintf(output, "Sensor size: %ld × %ld pixels\n",
            cam->sensorwidth, cam->sensorheight);
    andor_print_configuration(output, &cam->config);
    fprintf(output, "Supported pixel encodings: [");
    for (long k = 0; k < cam->nencodings; ++k) {
        if (k > 0) {
            fputs(", ", output);
        }
        fprintf(output, "%ls", andor_get_encoding_name(cam->encodings[k]));
    }
    fputs("]\n", output);
}
