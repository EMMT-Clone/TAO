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
#include <andor.h>
#include <andor-features.h>

int main(int argc, char* argv[])
{
    tao_error_t* errs = TAO_NO_ERRORS;
    andor_camera_t* cam;
    long dev, ndevices;
    char* end;

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
    fprintf(stdout, "Sensor size: %ld × %ld pixels\n",
            cam->sensorwidth, cam->sensorheight);
    fprintf(stdout, "Sensor temperature: %.1f°C\n", cam->config.temperature);
    fprintf(stdout, "Pixel binning: %ld×%ld\n",
            cam->config.xbin, cam->config.ybin);
    fprintf(stdout, "Region of interest: %ld×%ld at (%ld,%ld)\n",
            cam->config.width, cam->config.height,
            cam->config.xoff, cam->config.yoff);
    fprintf(stdout, "Exposure time: %g s\n", cam->config.exposuretime);
    fprintf(stdout, "Frame rate: %g Hz\n", cam->config.framerate);
    fprintf(stdout, "Supported pixel encodings: [");
    for (long k = 0; k < cam->nencodings; ++k) {
        if (k > 0) {
            fputs(", ", stdout);
        }
        fprintf(stdout, "%ls", andor_get_encoding_name(cam->encodings[k]));
    }
    fputs("]\n", stdout);

    andor_close_camera(cam);
    return EXIT_SUCCESS;
}
