/*
 * andor-test1.c --
 *
 * Simple tests for Andor cameras library: initialize library and query number
 * of available devices.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#include <stdio.h>
#include <andorcameras.h>

int main(int argc, char* argv[])
{
    tao_error_t* errs = TAO_NO_ERRORS;
    long ndevices = andor_get_ndevices(&errs);
    if (ndevices < 0) {
        tao_report_errors(&errs);
        return EXIT_FAILURE;
    }
    fprintf(stdout, "%ld device(s) found\n", ndevices);
    return EXIT_SUCCESS;
}
