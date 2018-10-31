/*
 * phx_info.c -
 *
 * Simple program to show the configuration of a camera connected to an
 * ActiveSilicon Phoenix frame grabber.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2017-2018, Éric Thiébaut.
 * Copyright (C) 2016, Éric Thiébaut & Jonathan Léger.
 */

#include <stdio.h>

#include "phoenix.h"

int main(int argc, char** argv)
{
    tao_error_t* errs = TAO_NO_ERRORS;
    phx_camera_t* cam;

    cam = phx_create(&errs, NULL, NULL, PHX_BOARD_NUMBER_AUTO);
    if (cam == NULL) {
        if (errs == TAO_NO_ERRORS) {
            fprintf(stderr, "Failed to create the camera.\n");
        } else {
            tao_report_errors(&errs);
        }
        return 1;
    }
    phx_print_camera_info(cam, stdout);
    phx_destroy(cam);
    return 0;
}
