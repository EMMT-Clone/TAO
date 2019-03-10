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
#include <stdbool.h>
#include <pthread.h>
#include <tao.h>
#include <phx_api.h>
#include <phoenix-types.h>
#include <coaxpress.h>

#if defined __cplusplus
extern "C" {
#elif 0
}
#endif

/**
 * @addtogroup PhoenixCameras
 *
 * Using cameras connected to ActiveSilicon *Phoenix* frame grabber.
 *
 * @{
 */

/*--------------------------------------------------------------------------*/
/* ERROR MANAGEMENT */

/**
 * @addtogroup PhoenixBasics
 *
 * Basic operations and type definitions for Phoenix cameras.
 *
 * @{
 */

/**
 * Structure describing a camera connected to an ActiveSilicon *Phoenix* frame
 * grabber.
 */
typedef struct phx_camera phx_camera_t;

/**
 * Virtual buffer used for real-time acquisition.
 */
typedef struct phx_virtual_buffer {
    void* data;
    int64_t counter;
    struct timespec ts;
    int index;
} phx_virtual_buffer_t;

/**
 * Connection settings for image transmission.
 */
typedef struct phx_connection {
    uint32_t channels; /**< Number of active connection channels */
    uint32_t speed;    /**< Bitrate (in Mbps) of each channel */
} phx_connection_t;

/**
 * Configuration settings.
 *
 * This structure stores all settable camera settings.
 */
typedef struct phx_config {
    double bias;           /**< Detector Bias */
    double gain;           /**< Detector gain */
    double exposure;       /**< Exposure time (in seconds) */
    double rate;           /**< Frames per seconds */
    phx_connection_t connection;
                           /**< Connection for image transmission */
    int depth;             /**< Bits per pixel */
    tao_image_roi_t roi;   /**< ROI of acquired images and defined relatively
                                to the sensor surface */
} phx_config_t;

/**
 * Camera structure.
 *
 * This structure is exposed for convenience but should be considered as
 * read-only by the user.
 */
struct phx_camera {
    pthread_mutex_t mutex;  /**< Lock to protect this structure */
    pthread_cond_t cond;    /**< Condition variable to signal events */
    tao_error_t* errs;      /**< Error stack */
    phx_handle_t handle;    /**< Camera handle */
    int (*initialize)(phx_camera_t*);
                            /**< Hook to (re-)initialize the camara */
    int (*start)(phx_camera_t*); /**< Start hook */
    int (*stop)(phx_camera_t*);  /**< Stop hook */
    int (*update_temperature)(phx_camera_t*);  /**< Update temperature hook */
    int (*update_config)(phx_camera_t*);
                            /**< Hook to retrieve the camera current device
                                 settings (and update dev_cfg) */
    int (*set_config)(phx_camera_t*, const phx_config_t*);
                            /**< Hook to set the camera settings according to
                                 the configuration chosen by the user */
    int (*save_config)(phx_camera_t*, int);
                            /**< Hook to save the camera settings */
    int (*load_config)(phx_camera_t*, int);
                            /**< Hook to load the camera settings */
    double temperature;     /**< Camera temperature (in degrees Celsius) */
    phx_config_t cfg;       /**< Current settings */
    tao_image_roi_t dev_roi;/**< ROI for the device */
    uint32_t fullwidth;     /**< Width (in pixels) of the sensor */
    uint32_t fullheight;    /**< Height (in pixels) of the sensor */
    uint32_t pixel_format;  /**< Raw pixel format of the camera */
    phx_value_t cam_color;  /**< Pixel format in acquired images
                                 (PHX_CAM_SRC_...) */
    phx_value_t buf_format; /**< Format of destination buffers
                                 (PHX_DST_FORMAT_...) */
    int state;              /**< Current state of the camera (> 1 if
                                 acquisition started). */

    /* Members for CoaXPress cameras. */
    bool coaxpress;        /**< Camera has CoaXPress connection */
    bool swap;             /**< Byteswapping needed for CoaXPress connection */
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
    long pending;          e /**< Number of pending image buffers */
    bool quitting;         /**< Acquisition is about to finish */
    size_t bufsize;        /**< Size of acquisition buffers */
    uint32_t events;       /**< Mask of events to be signaled */
};

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
 * Print camera description.
 */
extern int
phx_print_camera_info(phx_camera_t* cam, FILE* stream);

/**
 * Lock a camera.
 *
 * This function locks the mutex of the camera @b cam.  The caller is
 * responsible of eventually calling phx_unlock().
 *
 * Locking a camera is needed before changing anything in the camera instance
 * because the camera instance may be shared between the thread handling frame
 * grabber events and the other threads.  The following high level functions
 * take care of locking/unlocking the camera: phx_start(), phx_stop(),
 * phx_abort(), phx_wait(), phx_release_buffer(), phx_print_camera_info(),
 * phx_load_configuration(), phx_save_configuration(), phx_get_configuration(),
 * phx_set_configuration() and phx_update_configuration().  All other functions
 * which take a camera instance as their argument should only be called while
 * the caller has locked the camera.
 *
 * @param cam    Address of camera instance (must be non-`NULL`).
 */
extern void
phx_lock(phx_camera_t* cam);

/**
 * Try to lock a camera.
 *
 * This function attempts to lock the mutex of the camera @b cam.  If the call
 * is successful, the caller is responsible of eventually calling phx_unlock().
 *
 * @param cam    Address of camera instance (must be non-`NULL`).
 *
 * @return `1` if camera has been locked by the caller; `0` otherwise (that is,
 * if the camera is already locked by some other thread/process).
 */
extern int
phx_try_lock(phx_camera_t* cam);

/**
 * Unlock a camera.
 *
 * This function unlocks the mutex of the camera @b cam.  The caller must
 * have locked the camera @b cam with phx_lock() or phx_try_lock().
 *
 * @param cam    Address of camera instance (must be non-`NULL`).
 */
extern void
phx_unlock(phx_camera_t* cam);

/**
 * Signal the condition variable of a camera.
 *
 * This function restarts one of the threads that are waiting on the condition
 * variable of the camera @b cam.  Nothing happens, if no threads are waiting
 * on the condition variable of @b cam.  The caller is assumed to have locked
 * the camera before calling this function and to unlock the camera soon after
 * calling this function.
 *
 * @param cam    Address of camera instance (must be non-`NULL`).
 */
extern void
phx_signal_condition(phx_camera_t* cam);

/**
 * @}
 */

/*--------------------------------------------------------------------------*/
/* CONFIGURATION */

/**
 * @addtogroup PhoenixConfiguration
 *
 * Configuration of camera settings.
 *
 * Configurable camera parameters are the region of interest (ROI), the
 * exposure duration and frame rate, the detetctor bias and gain, *etc*.  As
 * setting camera parameters sequentially may result in an invalid
 * configuration, all parameters are set at the same time.  A complete camera
 * configuration is stored in a @ref phx_config_t structure.
 *
 * There are different possible operations with camera configuration:
 *
 * - **load** camera settings from a preset configuration;
 *
 * - **save** camera settings as a preset configuration;
 *
 * - **get** current configuration parameters;
 *
 * - **set** configuration parameters;
 *
 * - **update** configuration parameters.
 *
 * The *load* and *save* operations operate between the camera settings and a
 * given preset configuration stored in the ROM of the camera (or equivalent).
 * The *get* and *set* operations operate between the camera settings and a
 * user provided @ref phx_config_t structure.  For efficiency reasons, the
 * camera configuration is memorized in the @ref phx_camera_t structure
 * associated with the camera and is synchronized with the hardware settings.
 * It may however happen that errors prevent this synchronization.  In that
 * case, the *update* operation can be applied to synchronize the memorized
 * configuration with the current hardware settings.
 *
 * Due to hardware restrictions, the device ROI may be different than the one
 * chosen.  The device ROI is set to encompass the chosen ROI under these
 * restrictions.  The device ROI does not need to be accessible to the end-user
 * and is stored in the `dev_roi` member of the @ref phx_camera_t structure
 * while the chosen ROI is stored in the `cfg.roi` member of this structure.
 * After checking the validitity of the user chosen ROI, if an error occurs
 * when setting one the ROI parameter, the `cfg.roi` is reset to the hardware
 * settings.
 *
 * @{
 */

/**
 * Load camera settings.
 */
extern int
phx_load_configuration(phx_camera_t* cam, int id);

/**
 * Save camera settings.
 */
extern int
phx_save_configuration(phx_camera_t* cam, int id);

/**
 * Retrieve camera settings.
 */
extern void
phx_get_configuration(phx_camera_t* cam, phx_config_t* cfg);

/**
 * Set camera settings.
 */
extern int
phx_set_configuration(phx_camera_t* cam, const phx_config_t* cfg);

/**
 * Update camera settings to match hardware ones.
 */
extern int
phx_update_configuration(phx_camera_t* cam);

/**
 * Set camera connection settings.
 */
extern int
phx_set_coaxpress_connection(phx_camera_t* cam, const phx_connection_t* con);

/**
 * Update camera temperature.
 */
extern int
phx_update_temperature(phx_camera_t* cam);

/**
 * @}
 */

/*--------------------------------------------------------------------------*/
/* IMAGE ACQUISITION */

/**
 * @addtogroup PhoenixAcquisition
 *
 * Image acquisition with a Phoenix camera.
 * @{
 */

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
 * @return `0` on success, `-1` on failure.  Errors, if any, are stacked in
 *         camera instance.
 *
 * @see phx_stop(), phx_abort(), phx_wait(), phx_any_errors().
 */
extern int
phx_start(phx_camera_t* cam, int nbufs);

/**
 * Wait for an image to be available.
 *
 * @param cam    Address of camera instance.
 * @param secs   Maximum number of seconds to wait.  Must be nonnegative.
 *               Wait forever if @b secs is larger than one year.
 * @param drop   If true, the very last image is always delivered,  the
 *               older unprocessed images, if any, are discarded.
 *
 * @return The index (starting at 1) of the next image available in the ring of
 * virtual buffers.  0 is returned if timeout expired before a new image is
 * available or if acquisition of last image was aborted, -1 is returned in
 * case of errors.  If a valid index (>= 1) is returned, the caller shall use
 * the contents of the acquisition buffer as soon as possible and call
 * phx_release_buffer() when this is done.
 *
 * @see phx_release_buffer().
 */
extern int
phx_wait(phx_camera_t* cam, double secs, bool drop);

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

/**
 * @}
 */

/*--------------------------------------------------------------------------*/
/* ERROR MANAGEMENT */

/**
 * @addtogroup PhoenixErrors
 *
 * Management of errors with Phoenix cameras.
 *
 * The @ref phx_camera_t structure has its own error stack which is used by
 * most functions operating on this class of cameras to store error
 * information.
 *
 * The end user shall call phx_any_errors() to check whether there are any
 * errors, phx_report_errors() to print any errors to the standard error output
 * and phx_discard_errors() to discard any errors.  Note that all functions of
 * the Phoenix/TAO library return a value which can be checked to figure out
 * whether an error has occured.
 *
 * At low level, phx_push_error() is called to push a new error on top of the
 * error stack.  Errors occuring in functions of the ActiveSilicon Phoenix
 * library may also immediately print some error message.  The printing of such
 * messages is controlled by phx_set_error_handler_verbosity() and the current
 * settings are given by calling phx_get_error_handler_verbosity().
 *
 * @{
 */

/**
 * Check whether there are any errors with a camera.
 *
 * @warning The caller is responsible of locking/unlocking the camera.
 *
 * @todo As this is supposed to be a high level function, locking/unlocking the
 * camera should be done by this function.
 *
 * @param cam    Address of camera instance.
 *
 * @return `1` (true) if there are errors in the error stack of the camera @b
 * cam; `0` (false) otherwise.
 *
 * @see phx_create(), phx_report_errors(), phx_discard_errors().
 */
extern int
phx_any_errors(const phx_camera_t* cam);

/**
 * Report all tracked errors of a camera.
 *
 * This function prints to the standard error stream the errors in the error
 * stack of the camera @b cam, deleting the errors in the process.
 *
 * @warning The caller is responsible of locking/unlocking the camera.
 *
 * @todo As this is supposed to be a high level function, locking/unlocking the
 * camera should be done by this function.
 *
 * @param cam    Address of camera instance.
 *
 * @see tao_report_errors(), phx_create(), phx_any_errors(),
 * phx_discard_errors().
 */
extern void
phx_report_errors(phx_camera_t* cam);

/**
 * Clear all tracked errors of a camera.
 *
 * This function deletes the contents of the error stack of the camera @b cam.
 *
 * @warning The caller is responsible of locking/unlocking the camera.
 *
 * @todo As this is supposed to be a high level function, locking/unlocking the
 * camera should be done by this function.
 *
 * @param cam    Address of camera instance.
 *
 * @see tao_discard_errors(), phx_any_errors(), phx_report_errors(),
 * phx_create().
 */
extern void
phx_discard_errors(phx_camera_t* cam);

/**
 * Set the level of verbosity of the default error handler.
 *
 * @param level   The level of verbosity: < 1 to print no messages, 1 to print
 *                brief messages, > 1 to print detailed messages.
 *
 * @return The previous value of the verbosity level.
 */
extern int
phx_set_error_handler_verbosity(int level);

 /**
 * Get the level of verbosity of the default error handler.
 *
 * @return The current verbosity level for error messages printed by the
 *         default error handler.
 */
extern int
phx_get_error_handler_verbosity();

/**
 * Push a new error on top of the camera error stack.
 *
 * @warning The caller is responsible of locking/unlocking the camera.
 */
extern void
phx_push_error(tao_error_t** errs, const char* func, int code);

/**
 * Get textual identifier of status code.
 *
 * @param status    Status returned by one of the functions of the
 *                  ActiveSilicon Phoenix library.
 *
 * @return The address of a static string with the textual identifier of
 *         the status.
 */
extern const char*
phx_status_identifier(phx_status_t status);

/**
 * Get textual description of status code.
 *
 * @param status    Status returned by one of the functions of the
 *                  ActiveSilicon Phoenix library.
 *
 * @return The address of a static string describing the status code.
 */
const char*
phx_status_description(phx_status_t status);

/**
 * @}
 */

/*--------------------------------------------------------------------------*/
/* SUPPORTED CAMERAS */

/**
 * @addtogroup PhoenixSupportedCameras
 *
 * Supported cameras.
 *
 * Supported cameras are checked by `phx_check_MODEL()` and initialized
 * by `phx_initialize_MODEL()` where `MODEL` is the model of the cameras.
 *
 * @{
 */

/**
 * Check whether a camera is one of the Mikrotron MC408x cameras.
 *
 * @param cam   Reference to a structure describing a freshly connected camera.
 *
 * @return Non-zero if @b cam is connected to one of the Mikrotron MC408x
 * cameras; zero otherwise.
 *
 * @see phx_initialize_mikrotron_mc408x
 */
extern int
phx_check_mikrotron_mc408x(phx_camera_t* cam);

/**
 * Initialize a Mikrotron MC408x camera.
 *
 * This function initialize the members of @b cam assuming it is connected to
 * one of the Mikrotron MC408x cameras.
 *
 * @param cam   Reference to a structure describing a freshly connected camera.
 *
 * @return 0 on success, -1 on error (with errors information stored in
 * camera error stack).
 *
 * @see phx_check_mikrotron_mc408x.
 */
extern int
phx_initialize_mikrotron_mc408x(phx_camera_t* cam);

/**
 * @}
 */

/*---------------------------------------------------------------------------*/
/* GET/SET PHOENIX PARAMETERS AND READ/WRITE COAXPRESS REGISTERS */

/**
 * @addtogroup PhoenixLowLevel
 *
 * Low level functions for ActiveSilicon *Phoenix* frame grabber.
 *
 * These functions are to get or set ActiveSilicon Phoenix parameters and to
 * read or write CoaXPress registers.  These functions do not lock/unlock the
 * camera, it is the caller's responsibility to do it.
 *
 * @{
 */

/**
 * Execute a frame grabber command.
 */
extern int
phx_read_stream(phx_camera_t* cam, phx_acquisition_t command, void* addr);

/**
 * Query a frame grabber parameter.
 */
extern int
phx_get_parameter(phx_camera_t* cam, phx_param_t param, void* addr);

/**
 * Set a frame grabber parameter.
 */
extern int
phx_set_parameter(phx_camera_t* cam, phx_param_t param, void* addr);

/**
 * Query the value of a frame grabber parameter.
 */
extern int
phx_get(phx_camera_t* cam, phx_param_t param, phx_value_t* valptr);

/**
 * Set the value of a frame grabber parameter.
 */
extern int
phx_set(phx_camera_t* cam, phx_param_t param, phx_value_t value);

/**
 * Read data from a CoaXPress register.
 */
extern int
cxp_read(phx_camera_t* cam, uint32_t addr, uint8_t* data, uint32_t* size);

/**
 * Write data to a CoaXPress register.
 */
extern int
cxp_write(phx_camera_t* cam, uint32_t addr, uint8_t* data, uint32_t* size);

/**
 * Reset a CoaXPress register.
 *
 * @warning This functionality is not described in the ActiveSilicon manuals.
 */
extern int
cxp_reset(phx_camera_t* cam, uint32_t addr);

/**
 * Read an unsigned 32-bit integer from a CoaXPress register.
 */
extern int
cxp_read_uint32(phx_camera_t* cam, uint32_t addr, uint32_t* value);

/**
 * Read an unsigned 64-bit integer from a CoaXPress register.
 */
extern int
cxp_read_uint64(phx_camera_t* cam, uint32_t addr, uint64_t* value);

/**
 * Read a 32-bit floating-point value from a CoaXPress register.
 */
extern int
cxp_read_float32(phx_camera_t* cam, uint32_t addr, float32_t* value);

/**
 * Read a 64-bit floating-point value from a CoaXPress register.
 */
extern int
cxp_read_float64(phx_camera_t* cam, uint32_t addr, float64_t* value);

/**
 * Read a string from a CoaXPress register.
 */
extern int
cxp_read_string(phx_camera_t* cam, uint32_t addr, uint32_t len, char* buf);

/**
 * Indirect reading of an unsigned 32-bit integer from a CoaXPress register.
 */
extern int
cxp_read_indirect_uint32(phx_camera_t* cam, uint32_t addr, uint32_t* value);

/**
 * Write an unsigned 32-bit integer to a CoaXPress register.
 */
extern int
cxp_write_uint32(phx_camera_t* cam, uint32_t addr, uint32_t value);

/**
 * Write an unsigned 64-bit integer to a CoaXPress register.
 */
extern int
cxp_write_uint64(phx_camera_t* cam, uint32_t addr, uint64_t value);

/**
 * Write a 32-bit floating-point value to a CoaXPress register.
 */
extern int
cxp_write_float32(phx_camera_t* cam, uint32_t addr, float32_t value);

/**
 * Write a 64-bit floating-point value to a CoaXPress register.
 */
extern int
cxp_write_float64(phx_camera_t* cam, uint32_t addr, float64_t value);

/**
 * @}
 */

/*--------------------------------------------------------------------------*/
/* UTILITIES */

/**
 * @addtogroup PhoenixUtilities
 *
 * Utility functions for ActiveSilicon *Phoenix* frame grabber.
 *
 * @{
 */

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
 */
extern void phx_keyboard_init(void);

/**
 * Cross-platform routine to check whether keyboard hit occured.
 */
extern int  phx_keyboard_hit();

/**
 * Finalize cross-platform keyboard input routines.
 */
extern void phx_keyboard_final(void);

/**
 * Read a character from the keyboard.
 */
extern int phx_keyboard_read(void);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _PHOENIX_H */
