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

typedef struct andor_camera andor_camera_t;
typedef struct andor_camera_config andor_camera_config_t;
typedef struct andor_feature_value andor_feature_value_t;

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

extern void _andor_push_error(tao_error_t** errs, const char* func, int code);
extern void andor_push_error(andor_camera_t* cam, const char* func, int code);

typedef enum andor_feature_type {
    ANDOR_FEATURE_NOT_IMPLEMENTED = 0,
    ANDOR_FEATURE_BOOLEAN,
    ANDOR_FEATURE_INTEGER,
    ANDOR_FEATURE_FLOAT,
    ANDOR_FEATURE_ENUMERATED,
    ANDOR_FEATURE_STRING,
    ANDOR_FEATURE_COMMAND
} andor_feature_type_t;

/* R/W flags are shifted so that they can be combined with the feature
   type in a small integer value. */
#define ANDOR_FEATURE_READABLE    (1<<5)
#define ANDOR_FEATURE_WRITABLE    (1<<6)
#define ANDOR_FEATURE_TYPE_MASK   (ANDOR_FEATURE_READABLE - 1)

extern const wchar_t* andor_feature_names[];
extern const andor_feature_type_t andor_simcam_feature_types[];
extern const andor_feature_type_t andor_zyla_feature_types[];

extern const wchar_t** andor_get_feature_names();
extern const andor_feature_type_t* andor_get_simcam_feature_types();
extern const andor_feature_type_t* andor_get_zyla_feature_types();

typedef enum andor_camera_model {
    ANDOR_MODEL_UNKNOWN,
    ANDOR_MODEL_APOGEE,
    ANDOR_MODEL_BALOR,
    ANDOR_MODEL_ISTAR,
    ANDOR_MODEL_MARANA,
    ANDOR_MODEL_NEO,
    ANDOR_MODEL_SIMCAM,
    ANDOR_MODEL_SONA,
    ANDOR_MODEL_SYSTEM,
    ANDOR_MODEL_ZYLA
} andor_camera_model_t;

struct andor_camera_config {
    /* Region of interest. */
    long xbin;   /* AOIHBin (in pixels) */
    long ybin;   /* AOIVBin (in pixels) */
    long xoff;   /* AOILeft - 1 (in pixels) */
    long yoff;   /* AOITop -1 (in pixels) */
    long width;  /* AOIWidth (in macro-pixels) */
    long height; /* AOIHeight (in macro-pixels) */

    double exposuretime; /* Exposure time (in seconds) */
    double framerate;    /* Frame rate (in frames per second) */
    double temperature;  /* SensorTemperature */
};

struct andor_camera
{
    AT_H handle;
    tao_error_t* errs;
    int state; /* 0: not yet connected to device,
                  1: acquisition is not running,
                  2: acquisition is running */
    long sensorwidth;  /* Sensor width (in pixels) */
    long sensorheight; /* Sensor height (in pixels) */

    /* Camera configuration. */
    andor_camera_config_t config;

    /* Acquisition buffers. */
    void** bufs; /* Allocated acquisition buffers */
    long nbufs;  /* Number of buffers */
    long bufsiz; /* Buffer size (in bytes) */
    long stride; /* AOIStride (in bytes) */
};

extern andor_camera_t* andor_open_camera(tao_error_t** errs, long dev);
extern void andor_close_camera(andor_camera_t* cam);
extern int andor_start(andor_camera_t* cam, long nbufs);
extern int andor_stop(andor_camera_t* cam);

/**
 * Update camera settings
 *
 * This function updates the camera settings from the hardware.  If argument @a
 * all is true, all settings are updated; otherwise, only the settings that may
 * change without request, such as the temperature of the sensor, are updated.
 *
 * @param cam   The camera.
 * @param all   Update all parameters?
 *
 * @return `0` on success, `-1` on error.
 */
extern int andor_update_configuration(andor_camera_t* cam, bool all);

_TAO_END_DECLS

#endif /* _ANDOR_H */
