/*
 * tao-fits.h --
 *
 * Definitions for using FITS files in TAO (TAO is a library for Adaptive
 * Optics software).
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

#ifndef _TAO_FITS_H_
#define _TAO_FITS_H_ 1

/*
 * FITSIO header file is only included if the macro _FITSIO2_H is undefined.
 * This is to be able to compile TAO ebev though FITSIO is not available.
 */
#ifndef _FITSIO2_H
# include <fitsio2.h>
#endif

#include <stdbool.h>
#include <tao.h>

_TAO_BEGIN_DECLS

/**
 * @addtogroup Arrays
 * @{
 */

/**
 * Load a multi-dimensional array from a FITS file.
 *
 * This function loads the contents of a FITS IMAGE data into a new
 * multi-dimensional array.  The returned array has a reference count of 1, the
 * caller is responsible for unreferencing the array when no longer needed by
 * calling tao_unreference_array().
 *
 * @param errs      Address of a variable to track errors.
 * @param filename  Name of the FITS file.
 * @param extname   Name of the FITS extension to read.  Can be `NULL` to read
 *                  the first FITS IMAGE of the file.
 *
 * @return The address of a new multi-dimensional array; `NULL` in case of
 *         errors.
 *
 * @see tao_create_array(), tao_unreference_array(),
 *      tao_save_array_to_fits_file(), tao_load_array_from_fits_handle().
 */
extern tao_array_t*
tao_load_array_from_fits_file(tao_error_t** errs, const char* filename,
                              char* extname);

/**
 * Load a multi-dimensional array from a provided FITS handle.
 *
 * This function loads the contents of the current FITS HDU into a new
 * multi-dimensional array.  The returned array has a reference count of 1, the
 * caller is responsible for unreferencing the array when no longer needed by
 * calling tao_unreference_array().
 *
 * @param errs      Address of a variable to track errors.
 * @param fptr      FITS file handle.
 *
 * @return The address of a new multi-dimensional array; `NULL` in case of
 *         errors.
 *
 * @see tao_create_array(), tao_unreference_array(),
 *      tao_save_array_to_fits_handle(), tao_load_array_from_fits_file().
 */
extern tao_array_t*
tao_load_array_from_fits_handle(tao_error_t** errs, fitsfile* fptr);

/**
 * Save a multi-dimensional array to a FITS file.
 *
 * This function writes the contents of the supplied array into a new FITS
 * file.
 *
 * @param errs      Address of a variable to track errors.
 * @param arr       Pointer to an array referenced by the caller.
 * @param filename  FITS file handle.
 * @param overwrite Non-zero to allow for overwritting the destination.
 *
 * @return `0` in case of success, `-1` in case of failure.
 *
 * @see tao_create_array(), tao_unreference_array(),
 *      tao_save_array_to_fits_handle(), tao_load_array_from_fits_file().
 */
extern int
tao_save_array_to_fits_file(tao_error_t** errs, const tao_array_t* arr,
                            const char* filename, bool overwrite);

/**
 * Save a multi-dimensional array to a provided FITS handle.
 *
 * This function writes the contents of the supplied array into a new FITS HDU.
 * After having initialized the basic image information in the new HDU, a user
 * supplied callback function is called (if nont `NULL`) to let the caller
 * customize the keywords of the header.
 *
 * @param errs      Address of a variable to track errors.
 * @param arr       Pointer to an array referenced by the caller.
 * @param fptr      FITS file handle.
 * @param callback  Function called to update the header of the FITS HDU
 *                  where is written the array.  May be `NULL` to not use it.
 * @param ctx       Context argument supplied for the callback function.
 *
 * @return `0` in case of success, `-1` in case of failure.
 *
 * @see tao_create_array(), tao_unreference_array(),
 *      tao_save_array_to_fits_file(), tao_load_array_from_fits_handle().
 */
extern int
tao_save_array_to_fits_handle(tao_error_t** errs, const tao_array_t* arr,
                              fitsfile* fptr,
                              int (*callback)(tao_error_t**, fitsfile*, void*),
                              void* ctx);

/** @} */

_TAO_END_DECLS

#endif /* _TAO_FITS_H_ */
