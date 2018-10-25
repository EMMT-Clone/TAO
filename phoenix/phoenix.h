/*
 * phoenix.h --
 *
 * Definitions for TAO interface to ActiveSilicon Phoenix frame grabber.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2017-2018, Éric Thiébaut.
 * Copyright (C) 2016, Éric Thiébaut & Jonathan Léger.
 */

#ifndef _PHOENIX_H
#define _PHOENIX_H 1

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <tao.h>
#include <phx_api.h>
#include <phoenix-types.h>
#include <coaxpress.h>

typedef struct phx_camera phx_camera_t;

/**
 * Camera structure.
 *
 * This structure is exposed for convenience but should be considered as
 * read-only by the user.
 */
struct phx_camera {
    tao_error_t* errs;   /**< Error stack */
    phx_handle_t handle; /**< Camera handle */
    int (*start)(phx_camera_t*); /**< Start hook */
    int (*stop)(phx_camera_t*);  /**< Stop hook */
    uint32_t fullwidth;  /**< Width (in pixels) of the sensor */
    uint32_t fullheight; /**< Height (in pixels) of the sensor */
    uint32_t xoff;       /**< Horizontal offset of acquired images with
                              respect to sensor*/
    uint32_t yoff;       /**< Vertical offset of acquired images with
                              respect to sensor*/
    uint32_t width;      /**< Width (in macro-pixels) of acquired images */
    uint32_t height;     /**< Height (in macro-pixels) of acquired images */
    int state;           /**< Current state of the camera (> 1 if
                              acquisition started). */

    /* Members for CoaXPress cameras. */
    int coaxpress;       /**< Camera has CoaXPress connection */
    int swap;            /**< Byteswapping needed for CoaXPress connection */
    uint32_t timeout;    /**< CoaXPress connection timeout (milliseconds) */
    char vendor[CXP_DEVICE_VENDOR_NAME_LENGTH + 1];
    char model[CXP_DEVICE_MODEL_NAME_LENGTH + 1];

    /* Members shared with the acquisition callback. */
    uint64_t frames;       /**< Number of frames received so far */
    uint64_t overflows;    /**< Number of overflows */
    pthread_mutex_t mutex; /**< Mutex to protect shared data */
    pthread_cond_t cond;   /**< Condition variable to signal events */
};

#ifdef __cplusplus
extern "C" {
#endif

extern void
phx_push_error(tao_error_t** errs, const char* func, int code);

extern const char*
phx_status_string(char* buffer, phx_status_t status);

extern phx_camera_t*
phx_create(tao_error_t** errs,
           void (*handler)(const char*, phx_status_t, const char*),
           char* configname, phx_value_t boardnumber);

extern void
phx_destroy(phx_camera_t* cam);

extern int
phx_read_stream(phx_camera_t* cam, phx_acquisition_t command, void* addr);

extern int
phx_get_parameter(phx_camera_t* cam, phx_param_t param, void* addr);

extern int
phx_set_parameter(phx_camera_t* cam, phx_param_t param, void* addr);

extern int
phx_get(phx_camera_t* cam, phx_param_t param, phx_value_t* valptr);

extern int
phx_set(phx_camera_t* cam, phx_param_t param, phx_value_t value);


/*---------------------------------------------------------------------------*/
/* ROUTINES TO READ/WRITE COAXPRESS REGISTERS */

extern int
cxp_read(phx_camera_t* cam, uint32_t addr, uint8_t* data, uint32_t* size);

extern int
cxp_write(phx_camera_t* cam, uint32_t addr, uint8_t* data, uint32_t* size);

extern int
cxp_reset(phx_camera_t* cam, uint32_t addr);

extern int
cxp_read_uint32(phx_camera_t* cam, uint32_t addr, uint32_t* value);

extern int
cxp_read_uint64(phx_camera_t* cam, uint32_t addr, uint64_t* value);

extern int
cxp_read_float32(phx_camera_t* cam, uint32_t addr, float32_t* value);

extern int
cxp_read_float64(phx_camera_t* cam, uint32_t addr, float64_t* value);

extern int
cxp_read_string(phx_camera_t* cam, uint32_t addr, uint32_t len, char* buf);

extern int
cxp_read_indirect_uint32(phx_camera_t* cam, uint32_t addr, uint32_t* value);

extern int
cxp_write_uint32(phx_camera_t* cam, uint32_t addr, uint32_t value);

extern int
cxp_write_uint64(phx_camera_t* cam, uint32_t addr, uint64_t value);

extern int
cxp_write_float32(phx_camera_t* cam, uint32_t addr, float32_t value);

extern int
cxp_write_float64(phx_camera_t* cam, uint32_t addr, float64_t value);

/*--------------------------------------------------------------------------*/
/* CAMERAS */

/**
 * Check whether a camera is one of the Mikrotron MC408x cameras.
 *
 * @param cam   Camera instance which has already been connected.
 *
 * @return Non-zero if @p cam is connected to one of the Mikrotron MC408x
 * cameras; zero otherwise.
 *
 * @see phx_initialize_mikrotron_mc408x
 */
extern int
phx_check_mikrotron_mc408x(phx_camera_t* cam);

/**
 * Initialize a Mikrotron MC408x camera.
 *
 * This function initialize the members of @p cam assuming it is connected to
 * one of the Mikrotron MC408x cameras.
 *
 * @param cam   Camera instance which has already been connected.
 *
 * @return 0 on success, -1 on error (with errors information stored in
 * camera error stack).
 *
 * @see phx_check_mikrotron_mc408x.
 */
extern int
phx_initialize_mikrotron_mc408x(phx_camera_t* cam);

/*--------------------------------------------------------------------------*/
/* UTILITIES */

/**
 * Get number of bits for a given capture format.
 *
 * @param fmt   One of the `PHX_DST_FORMAT_*` value.
 *
 * @return The number of bits per pixel for the given capture format; 0 if
 * unknown.
 */
extern uint32_t
phx_capture_format_bits(phx_value_t fmt);

/**
 * Get pixel type for a given capture format.
 *
 * @param fmt   One of the `PHX_DST_FORMAT_*` value.
 *
 * @return The pixel type of the given capture format:
 *
 * - 0 if unknown;
 * - 1 if monochrome;
 * - 2 if Bayer format;
 * - 3 if RGB;
 * - 4 if BGR;
 * - 5 if RGBX;
 * - 6 if BGRX;
 * - 7 if XRGB;
 * - 8 if XBGR;
 * - 9 if YUV422;
 */
extern int
phx_capture_format_type(phx_value_t fmt);

/**
 * Initialize cross-platform keyboard input routines.
 *
 */
extern void phx_keyboard_init(void);

/**
 * Cross-platform routine to check whether keyboard hit occured.
 *
 */
extern int  phx_keyboard_hit();

/**
 * Finalize cross-platform keyboard input routines.
 *
 */
extern void phx_keyboard_final(void);

/**
 * Read a character from the keyboard.
 *
 */
extern int phx_keyboard_read(void);

#ifdef __cplusplus
}
#endif

#endif /* _PHOENIX_H */
