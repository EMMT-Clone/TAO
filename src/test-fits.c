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
#include <string.h>

#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#define MAX(a,b) ((a) >= (b) ? (a) : (b))

int
main(int argc, char* argv[])
{
    int pad = 0;
    int crop = 0;
    int quiet = 0;
    int convert = 0;
    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-pad") == 0) {
            pad = 1;
        } else if (strcmp(argv[i], "-crop") == 0) {
            crop = 1;
        } else if (strcmp(argv[i], "-convert") == 0) {
            convert = 1;
        } else if (strcmp(argv[i], "-quiet") == 0) {
            quiet = 1;
        } else {
            break;
        }
    }
    if (i != argc - 2) {
        fprintf(stderr, "usage: %s [-pad] [-crop] [-convert] [-quiet] SRC DST\n",
                argv[0]);
        return 1;
    }
    tao_array_t* arr = tao_load_array_from_fits_file(NULL, argv[i], NULL);
    if (arr == NULL) {
        /*
         * This should never happen because we are not providing an address to
         * record errors.
         */
        return 1;
    }
    if (pad || crop || convert) {
        long dstdims[TAO_MAX_NDIMS];
        long dstoffs[TAO_MAX_NDIMS];
        long srcdims[TAO_MAX_NDIMS];
        long srcoffs[TAO_MAX_NDIMS];
        long lens[TAO_MAX_NDIMS];
        int ndims = tao_get_array_ndims(arr);
        for (int d = 0; d < ndims; ++d) {
            srcdims[d] = tao_get_array_size(arr, d+1);
        }
        for (int d = 0; d < ndims; ++d) {
            srcoffs[d] = (crop ? srcdims[d]/4 : 0);
            dstoffs[d] = (pad ? srcdims[d]/4 : 0);
            dstdims[d] = (crop && pad ? srcdims[d] :
                          (crop && !pad ? 3*srcdims[d]/4 :
                           (!crop && pad ? 5*srcdims[d]/4 : srcdims[d])));
            lens[d] = MAX(dstdims[d] - dstoffs[d],
                          srcdims[d] - srcoffs[d]);
            if (! quiet) {
                printf("%d: srcoff=%ld srcdim=%ld dstoff=%ld dstdim=%ld len=%ld\n", d, srcoffs[d], srcdims[d], dstoffs[d], dstdims[d], lens[d]);
            }
        }
        tao_element_type_t eltype = tao_get_array_eltype(arr);
        if (convert) {
            if (eltype == TAO_FLOAT32) {
                eltype = TAO_FLOAT64;
            } else {
                eltype = TAO_FLOAT32;
            }
        }
        tao_array_t* tmp = arr;
        arr = tao_create_array(NULL, eltype, ndims, dstdims);
        memset(tao_get_array_data(arr), 0,
               tao_get_array_length(arr)*tao_get_element_size(eltype));
        tao_copy_array_to_array(NULL, arr, dstoffs, tmp, srcoffs, lens, ndims);
        tao_unreference_array(tmp);
    }
    int code = tao_save_array_to_fits_file(NULL, arr, argv[i+1], 0);
    if (code != 0) {
        /*
         * This should never happen because we are not providing an address to
         * record errors.
         */
        return 1;
    }
    tao_unreference_array(arr);
    return 0;
}
