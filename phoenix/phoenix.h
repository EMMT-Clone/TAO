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

typedef struct phx_virtual_buffer phx_virtual_buffer_t;

struct phx_virtual_buffer {
    void* data;
    int64_t counter;
    struct timespec ts;
    int index;
};

/**
 * Region of interest.
 *
 * A region of interest (ROI for short) is defined with respect to a *parent*.
 * This parent can be another ROI, the sensor, an image, etc.
 */
typedef struct phx_roi {
    int xoff;    /**< Horizontal offset with respect to parent */
    int yoff;    /**< Vertical offset with respect to parent */
    int width;   /**< Horizontal size */
    int height;  /**< Vertical size */
} phx_roi_t;

typedef struct phx_config {
    double bias;           /**< Detector Bias */
    double gain;           /**< Detector gain */
    double exposure;       /**< Exposure time (in seconds) */
    double rate;           /**< Frames per seconds */
    int depth;             /**< Bits per pixel */
    phx_roi_t roi;         /**< ROI of acquired images and defined relatively
                                to the sensor surface */
} phx_config_t;

/**
 * Camera structure.
 *
 * This structure is exposed for convenience but should be considered as
 * read-only by the user.
 */
struct phx_camera {
    pthread_mutex_t mutex; /**< Lock to protect this structure */
    pthread_cond_t cond;   /**< Condition variable to signal events */
    tao_error_t* errs;     /**< Error stack */
    phx_handle_t handle;   /**< Camera handle */
    int (*start)(phx_camera_t*); /**< Start hook */
    int (*stop)(phx_camera_t*);  /**< Stop hook */
    int (*update_temperature)(phx_camera_t*);  /**< Update temperature hook */
    int (*set_config)(phx_camera_t*);
                           /**< Hook to set the camera settings according to
                                the configuration chosen by the user */
    int (*get_config)(phx_camera_t*);
                           /**< Hook to get the camera settings according to
                                the configuration chosen by the user */
    double temperature;    /**< Camera temperature (in degrees Celsius) */
    phx_config_t dev_cfg;  /**< Current device settings */
    phx_config_t usr_cfg;  /**< User chosen camera settings */
    uint32_t pixelformat;  /**< Raw pixel format of the camera */
    uint32_t fullwidth;    /**< Width (in pixels) of the sensor */
    uint32_t fullheight;   /**< Height (in pixels) of the sensor */
    uint32_t srcdepth;     /**< Bits per pixel in acquired images */
    phx_roi_t cam_roi;     /**< ROI defining the images sent by the camera and
                                defined relatively to the sensor surface */
    phx_roi_t usr_roi;     /**< User chosen ROI */
    phx_value_t srcformat; /**< Pixel format in acquired images
                                (PHX_CAM_SRC_...) */
    phx_value_t dstformat; /**< Format of destination buffers
                                (PHX_DST_FORMAT_...) */
    int state;             /**< Current state of the camera (> 1 if
                                acquisition started). */

    /* Members for CoaXPress cameras. */
    int coaxpress;         /**< Camera has CoaXPress connection */
    int swap;              /**< Byteswapping needed for CoaXPress connection */
    uint32_t timeout;      /**< CoaXPress connection timeout (milliseconds) */
    char vendor[CXP_DEVICE_VENDOR_NAME_LENGTH + 1];
    char model[CXP_DEVICE_MODEL_NAME_LENGTH + 1];

    /* Members shared with the acquisition callback. */
    uint64_t frames;       /**< Number of frames received so far */
    uint64_t lostframes;   /**< Number of lost frames */
    uint64_t overflows;    /**< Number of overflows */
    int64_t lostsyncs;     /**< Number of synchronization losts so far */
    phx_imgbuf_t* bufs;    /**< Image buffers for acquisition */
    int nbufs;             /**< Number of acquisition buffers */
    int last;              /**< Index of last captured image buffer */
    int pending;           /**< Number of pending image buffers */
    size_t bufsize;        /**< Size of acquisition buffers */
    uint32_t events;       /**< Mask of events to be signaled */
};

#ifdef __cplusplus
extern "C" {
#endif

extern void
phx_push_error(tao_error_t** errs, const char* func, int code);

extern const char*
phx_status_string(phx_status_t status);

/**
 * Create a new instance of a frame grabber connection to a camera.
 *
 * This function allocates a camera hanlder, connects to the camera and sets
 * some initial camera parameters.  A pointer to a `phx_camera_t` structure is
 * returned.  This pointer can be used to query/set camera parameters and
 * acquire images.  When no longer needed, the ressources should be released by
 * calling phx_destroy().
 *
 * @param errs        A pointer to a variable to track errors.
 *
 * @param handler     A user defined handler for errors.  If `NULL`, a
 *                    default handler is used whose behavior can be tuned via
 *                    phx_set_error_handler_verbosity().
 *
 * @param configname  The name of the camera configuration file.  Can be
 *                    `NULL`, to attempt to automatically guess the camera
 *                    settings (for now, this only works for the Mikrotron
 *                    MC408x series cameras).
 *
 * @param boardnumber The frame grabber board number.  It can be automatically
 *                    set by using the value `PHX_BOARD_NUMBER_AUTO`.
 *
 * @return A pointer to the new instance or `NULL` in case of errors.
 *
 * @see phx_destroy().
 */
extern phx_camera_t*
phx_create(tao_error_t** errs,
           void (*handler)(const char*, phx_status_t, const char*),
           char* configname, phx_value_t boardnumber);

/**
 * Release the ressources associated to a frame grabber connection to a camera.
 *
 * This function aborts any acquisition with the associated camera and releases
 * all related ressources.
 *
 * @param camera      A pointer to the camera instance.
 *
 * @see phx_create().
 */
extern void
phx_destroy(phx_camera_t* cam);

/**
 * Start continuous acquisition.
 *
 * This function starts continuous acquisition with a given number of virtual
 * image buffers.
 *
 * @param cam    Address of camera instance.
 * @param nbufs  Number of virtual buffers (must be larger than 2 to avoid
 *               oveflows).
 *
 * @return `0` on success, `-1` on failure.
 *
 * @see phx_stop(), phx_abort(), phx_wait().
 */
extern int
phx_start(phx_camera_t* cam, int nbufs);

/**
 * Wait for an image to be available.
 *
 * @param cam    Address of camera instance.
 * @param secs   Maximum number of seconds to wait.  Must be nonnegative.
 *               Wait forever if @p secs is larger than one year.
 * @param drop   If non-zero, the very last image is always delivered,  the
 *               older unprocessed images, if any, are discarded.
 *
 * @return The index (starting at 1) of the next image available in the ring of
 * virtual buffers.  0 is returned if timeout expired before a new image is
 * available, -1 is returned in case of errors.  If a valid index (>= 1) is
 * returned, the caller shall use the contents of the acquisition buffer as
 * soon as possible and call phx_release_buffer() when this is done.
 *
 * @see phx_release_buffer().
 */
extern int
phx_wait(phx_camera_t* cam, double secs, int drop);

/**
 * Release usage of acquisition buffer.
 *
 * This function releases the usage of the last acquisition buffer and
 * decrement the number of pending frames.  This function must be called after
 * a successfull call to phx_wait().
 *
 * @param cam    Address of camera instance.
 *
 * @return `0` on success, `-1` on failure.
 *
 * @see phx_wait().
 */
extern int
phx_release_buffer(phx_camera_t* cam);

/**
 * Stop continuous acquisition after current image.
 *
 * This function stops continuous acquisition after current image has been
 * acquired.
 *
 * @param cam    Address of camera instance.
 *
 * @return `0` on success, `-1` on failure.
 *
 * @see phx_start(), phx_abort(), phx_wait().
 */
extern int
phx_stop(phx_camera_t* cam);

/**
 * Stop continuous acquisition immediately.
 *
 * This function stops continuous acquisition without waiting for the arrival
 * of the current image.
 *
 * @param cam    Address of camera instance.
 *
 * @return `0` on success, `-1` on failure.
 *
 * @see phx_start(), phx_stop(), phx_wait().
 */
extern int
phx_abort(phx_camera_t* cam);

extern int
phx_update_temperature(phx_camera_t* cam);

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
