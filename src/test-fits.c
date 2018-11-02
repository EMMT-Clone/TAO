/*
 * test-fits.c --
 *
 * Simple test for loading/saving of FITS files with TAO.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <tao.h>

int
main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s SRC DST\n", argv[0]);
        return 1;
    }
    tao_array_t* arr = tao_load_array_from_fits_file(NULL, argv[1], NULL);
    if (arr == NULL) {
        /*
         * This should never happen because we are not providing an address to
         * record errors.
         */
        return 1;
    }
    int code = tao_save_array_to_fits_file(NULL, arr, argv[2], 0);
    if (code != 0) {
        /*
         * This should never happen because we are not providing an address to
         * record errors.
         */
        return 1;
    }
    return 0;
}
