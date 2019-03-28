/*
 * andor-test3.c --
 *
 * Simple tests for Andor cameras library: open a camera and list its features.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <tao.h>
#include <andor.h>
#include <andor-features.h>

int main(int argc, char* argv[])
{
    tao_error_t* errs = TAO_NO_ERRORS;
    andor_camera_t* cam;
    long dev, ndevices;
    char* end;
    struct timespec t0, t1;

    ndevices = andor_get_ndevices(&errs);
    if (ndevices < 0) {
        tao_report_errors(&errs);
        return EXIT_FAILURE;
    }
    if (argc == 1) {
        dev = 0;
    } else if (argc == 2) {
        dev = strtol(argv[1], &end, 0);
        if (end == argv[1] || *end != '\0' || dev < 0 || dev >= ndevices) {
            fprintf(stderr, "Invalid device number \"%s\"\n", argv[1]);
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Usage: %s [dev]\n", argv[0]);
        return EXIT_FAILURE;
    }
    cam = andor_open_camera(&errs, dev);
    if (cam == NULL) {
        tao_report_errors(&errs);
        return EXIT_FAILURE;
    }
    andor_print_camera_configuration(stdout, cam);

    /* Measure time taken for updating all the configuration. */
    tao_get_monotonic_time(NULL, &t0);
    andor_update_configuration(cam, true);
    tao_get_monotonic_time(NULL, &t1);
    fprintf(stdout, "Time to update configuration: %.3f µs\n",
            1E6*tao_time_to_seconds(tao_subtract_times(&t1, &t1, &t0)));

    andor_close_camera(cam);
    return EXIT_SUCCESS;
}
