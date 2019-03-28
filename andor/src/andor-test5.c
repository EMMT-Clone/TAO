/*
 * andor-test5.c --
 *
 * Simple tests for Andor cameras library: open a camera, list its features
 * and acquire a bunch of images.
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
#include <tao-fits.h>
#include <andor.h>
#include <andor-features.h>

static double elapsed_time(const struct timespec* t1,
                           const struct timespec* t0)
{
    struct timespec dt;
    return tao_time_to_seconds(tao_subtract_times(&dt, t1, t0));
}

int main(int argc, char* argv[])
{
    tao_error_t* errs = TAO_NO_ERRORS;
    tao_array_t* arr;
    andor_camera_t* cam;
    andor_camera_config_t cfg;
    long dev, ndevices, nimgs, imgsiz;
    uint16_t* imgptr;
    char* end;
    struct timespec t0, t1;
    int status;

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
            1E6*elapsed_time(&t1, &t0));

    andor_get_configuration(cam, &cfg);
    //cfg.xbin = 1;
    //cfg.ybin = 1;
    //cfg.xoff = 300;
    //cfg.yoff = 200;
    //cfg.width = 640;
    //cfg.height = 480;
    cfg.framerate = 40.0;
    cfg.exposuretime = 0.005;
    cfg.pixelencoding = ANDOR_ENCODING_MONO12PACKED;
    if (andor_set_configuration(cam, &cfg) != 0) {
        andor_report_errors(cam);
        return EXIT_FAILURE;
    }
    andor_print_camera_configuration(stdout, cam);
    andor_get_configuration(cam, &cfg);

    /* Allocate an array to save some images. Errors are considered as
       fatal. */
    nimgs = 5;
    arr = tao_create_3d_array(NULL, TAO_INT16, cfg.width, cfg.height, nimgs);
    imgsiz = cfg.width*cfg.height;
    imgptr = (uint16_t*)tao_get_array_data(arr);

    /* Start acquisition. */
    tao_get_monotonic_time(NULL, &t0);
    if (andor_start_acquisition(cam, 4) != 0) {
        andor_report_errors(cam);
        return EXIT_FAILURE;
    }
    tao_get_monotonic_time(NULL, &t1);
    fprintf(stdout, "Time to start acquisition: %.3f ms\n",
            1E3*elapsed_time(&t1, &t0));

    /* Acquire some images. */
    int j = 0;
    for (int k = 0; k < 100; ++k) {
        void* buf;
        long siz;
        t0 = t1;
        status = andor_wait_buffer(cam, &buf, &siz, 0.1);
        tao_get_monotonic_time(NULL, &t1);
        if (status == 1) {
            fprintf(stdout, "%3d: %10.3f ms\n", k+1,
                    1E3*elapsed_time(&t1, &t0));
            if (j < nimgs) {
                andor_convert_buffer(imgptr + j*imgsiz,
                                     ANDOR_ENCODING_MONO16,
                                     buf, cfg.pixelencoding,
                                     cfg.width, cfg.height, cam->stride);
                ++j;
            }
            if (andor_queue_buffer(cam, buf, siz) != 0) {
                andor_report_errors(cam);
                (void)andor_stop_acquisition(cam);
                return EXIT_FAILURE;
            }
        } else if (status == 0) {
            fprintf(stderr, "%3d: Timeout!\n", k+1);
        } else {
            andor_report_errors(cam);
            (void)andor_stop_acquisition(cam);
            return EXIT_FAILURE;
        }
    }

    if (andor_stop_acquisition(cam) != 0) {
        andor_report_errors(cam);
        return EXIT_FAILURE;
    }

    tao_save_array_to_fits_file(NULL, arr, "/tmp/andor-test5.fits", true);
    tao_unreference_array(arr);
    andor_close_camera(cam);
    return EXIT_SUCCESS;
}
