#ifndef _ANDORCAMERAS_H
#define _ANDORCAMERAS_H 1

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
extern long
andor_get_ndevices(tao_error_t** errs);

extern const char*
andor_get_error_reason(int code);

extern const char*
andor_get_error_name(int code);

extern void
andor_push_error(tao_error_t** errs, const char* func, int code);

_TAO_END_DECLS

#endif /* _ANDORCAMERAS_H */
