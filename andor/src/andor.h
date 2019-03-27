/*
 * andor.h --
 *
 * Definitions for Andor cameras library.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#ifndef _ANDOR_H
#define _ANDOR_H 1

#include <stdlib.h>
#include <stdbool.h>
#include <atcore.h>
#include <tao.h>

_TAO_BEGIN_DECLS

/**
 * Initialize AndorCameras internals.
 *
 * This function initializes the Andor Core Library, retireve the number of
 * available devices and manage to automatically finalize the Andor Core
 * Library at the exit of the program.
 *
 * In principle, the application is supposed to call this function once before
 * calling other functions provided by AndorCameras.  However, calling this
 * function more than once has no effects and the functions provided by
 * AndorCameras are able to automatically call this function.
 *
 * @param errs   Address of a variable to track errors.
 *
 * @return `0` on success, `-1` on error.
 */
extern int
andor_initialize(tao_error_t** errs);

/**
 * Get the number of available devices.
 *
 * This function yelds the number of available Andor Camera devices.
 * If the Andor Core Library has not been initialized, andor_initialize()
 * is automatically called.
 *
 * @param errs   Address of a variable to track errors.
 *
 * @return The number of available devices, `-1L` on error.
 */
extern long andor_get_ndevices(tao_error_t** errs);

/**
 * Get the version of the Andor SDK.
 *
 * This function never fails, in case of errors, the returned version number is
 * `"0.0.0"` and errors messages are printed.
 *
 * @return A string.
 */
const char* andor_get_software_version();

extern const char* andor_get_error_reason(int code);

extern const char* andor_get_error_name(int code);

extern void andor_push_error(tao_error_t** errs, const char* func, int code);

typedef enum andor_feature_type {
    ANDOR_NOT_IMPLEMENTED,
    ANDOR_BOOLEAN,
    ANDOR_INTEGER,
    ANDOR_FLOAT,
    ANDOR_ENUMERATED,
    ANDOR_STRING,
    ANDOR_COMMAND
} andor_feature_type_t;

typedef struct andor_feature_value andor_feature_value_t;
struct andor_feature_value {
    andor_feature_type_t type;
    union {
        bool boolean;
        long integer;
        double floatingpoint;
        wchar_t* string;
    } value;
};

extern const wchar_t* andor_feature_names[];
extern const andor_feature_type_t andor_simcam_feature_types[];
extern const andor_feature_type_t andor_zyla_feature_types[];

extern const wchar_t** andor_get_feature_names();
extern const andor_feature_type_t* andor_get_simcam_feature_types();
extern const andor_feature_type_t* andor_get_zyla_feature_types();

typedef struct andor_camera andor_camera_t;
struct andor_camera
{
    AT_H handle;
    tao_error_t* errs;
    long sensorwidth;
    long sensorheight;
};

extern andor_camera_t* andor_open_camera(tao_error_t** errs, long dev);
extern void andor_close_camera(andor_camera_t* cam);

_TAO_END_DECLS

#endif /* _ANDOR_H */
