/*
 * phx_test.c -
 *
 * Program to test a camera connected to an ActiveSilicon Phoenix frame
 * grabber.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "phoenix.h"

static const char* progname = "phx_test";

static void
fatal(const char* format, ...)
{
    va_list ap;
    size_t len;
    if (progname != NULL && progname[0] != '\0') {
        (void)fprintf(stderr, "%s: ", progname);
    }
    va_start(ap, format);
    (void)vfprintf(stderr, format, ap);
    va_end(ap);
    len = strlen(format);
    if (len < 1 || format[len - 1] != '\n') {
        (void)fputs("\n", stderr);
    }
    (void)fflush(stderr);
    exit(1);
}

int
main(int argc, char* argv[])
{
    tao_error_t* errs = TAO_NO_ERRORS;
    phx_camera_t* cam = NULL;
    phx_config_t cfg;
    int load_id = -1;
    int save_id = -1;
    int quiet = 0;
    char c;
    char buffer[30];
    long count = 1;
    long skip = 2;
    int nbufs = 4;
    int drop = 1;
    double timeout = 10.0;

    /*
     * Parse the command line options and configure the camera.  Two passes are
     * needed: first pass to get the configuration to load, second pass (after
     * openning the camera) to set the options and, possibly, to save the
     * configuration.
     */
    for (int pass = 1; pass <= 2; ++pass) {
        int i = 0;
        while (++i < argc) {
            const char* arg = argv[i];
            if (arg[0] != '-') {
                break;
            }
            if (strcmp(arg, "--") == 0) {
                ++i;
                break;
            }
            if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
                if (pass != 2) {
                    continue;
                }
                fprintf(stderr, "Usage: %s [OPTIONS] [--]\n", progname);
                fprintf(stderr,
                        "Change and/or show camera configuration.\n");
                fprintf(stderr, "Options:\n");
                fprintf(stderr, "  --roi XOFF,YOFF,WIDTH,HEIGHT  "
                        "Region of interest [");
                fprintf(stderr, "%d,%d,%d,%d",
                        cfg.roi.xoff, cfg.roi.yoff,
                        cfg.roi.width, cfg.roi.height);
                fputs("].\n", stderr);
                fprintf(stderr, "  --load ID                     "
                        "Load configuration.\n");
                fprintf(stderr, "  --save ID                     "
                        "Save configuration.\n");
                fprintf(stderr, "  --depth BITS                  "
                        "Bits per pixel [%d bits].\n", cfg.depth);
                fprintf(stderr, "  --rate FPS                    "
                        "Frames per second [%g Hz].\n", cfg.rate);
                fprintf(stderr, "  --exposure TIME               "
                        "Exposure duration in seconds [%g s].\n",
                        cfg.exposure);
                fprintf(stderr, "  --bias LEVEL                  "
                        "Black level [%g].\n", cfg.bias);
                fprintf(stderr, "  --gain VALUE                  "
                        "Gain [%g].\n", cfg.gain);
                if (cfg.connection.speed != 0) {
                    sprintf(buffer, "%u Mbps",
                            (unsigned int)cfg.connection.speed);
                } else {
                    strcpy(buffer, "auto");
                }
                fprintf(stderr, "  --bitrate VALUE|auto          "
                        "CoaXPress bitrate [%s].\n", buffer);
                if (cfg.connection.channels != 0) {
                    sprintf(buffer, "%u",
                            (unsigned int)cfg.connection.channels);
                } else {
                    strcpy(buffer, "auto");
                }
                fprintf(stderr, "  --channels NUMBER|auto        "
                        "Number of CoaXPress channels [%s].\n", buffer);
                fprintf(stderr, "  --quiet                       "
                        "Quiet (non-verbose) mode.\n");
                fprintf(stderr, "  --timeout SECS                "
                        "Timeout for each frame in seconds [%g s].\n",
                        timeout);
                fprintf(stderr, "  --nbufs NUMBER                "
                        "Number of acquisition buffers [%d].\n", nbufs);
                fprintf(stderr, "  --count NUMBER                "
                        "Number of frames [%ld].\n", count);
                fprintf(stderr, "  --skip NUMBER                "
                        "Number of frames to skip [%ld].\n", skip);
                fprintf(stderr, "  -h, --help                    "
                        "Print this help.\n");
                exit(0);
            } else if (strcmp(arg, "--load") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--load`");
                }
                if (pass != 1) {
                    continue;
                }
                if (sscanf(argv[i], "%d %c", &load_id, &c) != 1 ||
                    load_id < 0) {
                    fatal("bad argument for option `--load ID`");
                }
            } else if (strcmp(arg, "--save") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--save`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], "%d %c", &save_id, &c) != 1 ||
                    save_id < 0) {
                    fatal("bad argument for option `--save ID`");
                }
            } else if (strcmp(arg, "--roi") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--roi`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %d , %d , %d , %d %c",
                           &cfg.roi.xoff, &cfg.roi.yoff,
                           &cfg.roi.width, &cfg.roi.height, &c) != 4) {
                    fatal("bad argument for option `--roi`, "
                          "should be `XOFF,YOFF,WIDTH,HEIGHT`");
                }
            } else if (strcmp(arg, "--depth") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--depth`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %d %c", &cfg.depth, &c) != 1 ||
                    cfg.depth < 1) {
                    fatal("bad value for option `--depth`");
                }
            } else if (strcmp(arg, "--rate") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--rate`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %lf %c", &cfg.rate, &c) != 1 ||
                    cfg.rate <= 0 || cfg.rate > INT32_MAX) {
                    fatal("bad value for option `--rate`");
                }
            } else if (strcmp(arg, "--exposure") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--exposure`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %lf %c", &cfg.exposure, &c) != 1 ||
                    cfg.exposure < 0 || cfg.exposure > INT32_MAX) {
                    fatal("bad value for option `--exposure`");
                }
            } else if (strcmp(arg, "--bias") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--bias`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %lf %c", &cfg.bias, &c) != 1 ||
                    cfg.bias < 0) {
                    fatal("bad value for option `--bias`");
                }
            } else if (strcmp(arg, "--gain") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--gain`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %lf %c", &cfg.gain, &c) != 1 ||
                    cfg.gain < 1) {
                    fatal("bad value for option `--gain`");
                }
            } else if (strcmp(arg, "--bitrate") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--bitrate`");
                }
                if (pass != 2) {
                    continue;
                }
                if (strcmp(argv[i], "auto") == 0) {
                    cfg.connection.speed = 0;
                } else {
                    unsigned int bitrate;
                    if (sscanf(argv[i], " %u %c", &bitrate, &c) != 1 ||
                        bitrate < 1) {
                        fatal("bad value for option `--bitrate`");
                    }
                    cfg.connection.speed = bitrate;
                }
            } else if (strcmp(arg, "--channels") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--channels`");
                }
                if (pass != 2) {
                    continue;
                }
                if (strcmp(argv[i], "auto") == 0) {
                    cfg.connection.channels = 0;
                } else {
                    unsigned int channels;
                    if (sscanf(argv[i], " %u %c", &channels, &c) != 1 ||
                        channels < 1 || channels > 4) {
                        fatal("bad value for option `--channels`");
                    }
                    cfg.connection.channels = channels;
                }
            } else if (strcmp(arg, "--timeout") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--timeout`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %lf %c", &timeout, &c) != 1 ||
                    isnan(timeout) || timeout < 0) {
                    fatal("bad value for option `--timeout`");
                }
            } else if (strcmp(arg, "--nbufs") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--nbufs`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %d %c", &nbufs, &c) != 1 ||
                    nbufs < 2) {
                    fatal("bad value for option `--nbufs`");
                }
            } else if (strcmp(arg, "--count") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--count`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %ld %c", &count, &c) != 1 ||
                    count < 0) {
                    fatal("bad value for option `--count`");
                }
            } else if (strcmp(arg, "--skip") == 0) {
                if (++i >= argc) {
                    fatal("missing argument to option `--skip`");
                }
                if (pass != 2) {
                    continue;
                }
                if (sscanf(argv[i], " %ld %c", &skip, &c) != 1 ||
                    skip < 0) {
                    fatal("bad value for option `--skip`");
                }
            } else if (strcmp(arg, "--quiet") == 0) {
                quiet = 1;
            } else {
                fatal("unknown option `%s`", argv[i]);
            }
        }
        if (i < argc) {
            fatal("too many arguments");
        }
        if (pass == 1) {
            /* Open the camera and retrieve its configuration. */
            cam = phx_create(&errs, NULL, NULL, PHX_BOARD_NUMBER_AUTO);
            if (cam == NULL) {
                if (errs == TAO_NO_ERRORS) {
                    fprintf(stderr, "Failed to create the camera.\n");
                } else {
                    tao_report_errors(&errs);
                }
                return 1;
            }
            if (load_id >= 0 && phx_load_configuration(cam, load_id) != 0) {
                if (phx_any_errors(cam)) {
                    phx_report_errors(cam);
                } else {
                    fprintf(stderr, "Failed to load preset configuration.\n");
                }
                return 1;
            }
            phx_get_configuration(cam, &cfg);
        } else if (pass == 2) {
            /* Configure the camera according to options. */
            if (phx_set_configuration(cam, &cfg) != 0) {
                if (phx_any_errors(cam)) {
                    phx_report_errors(cam);
                } else {
                    fprintf(stderr, "Failed to configure the camera.\n");
                }
                return 1;
            }
            if (save_id >= 0 && phx_save_configuration(cam, save_id) != 0) {
                if (phx_any_errors(cam)) {
                    phx_report_errors(cam);
                } else {
                    fprintf(stderr, "Failed to save preset configuration.\n");
                }
                return 1;
            }
        }
    }

    if (! quiet) {
        /* Print informations about the camera. */
        if (phx_print_camera_info(cam, stdout) != 0) {
            phx_report_errors(cam);
            return 1;
        }
    }

    if (count > 0) {
        long errors = 0;
        long frames = 0;
        long timeouts = 0;
        int previous = -1;
        int stepping = 0;
        fprintf(stdout, "Starting...\n");
        if (phx_start(cam, nbufs) != 0) {
            phx_report_errors(cam);
            return 1;
        }
        fprintf(stdout, "Acquiring...\n");
        while (1) {
            int index = phx_wait(cam, timeout, drop);
            if (index < 0) {
            error:
                if (phx_abort(cam)!= 0) {
                    ++errors;
                }
                ++errors;
                break;
            } else if (index == 0) {
                if (cam->quitting) {
                stop:
                    if (phx_stop(cam) != 0) {
                        ++errors;
                    }
                    break;
                }
                ++timeouts;
            } else {
                if (previous > 0) {
                    int step = (nbufs + (index - previous))%nbufs;
                    if (step != stepping) {
                        stepping = step;
                        fprintf(stdout, "Stepping: %d buffer(s)\n", stepping);
                    }
                }
                if (skip > 0) {
                    --skip;
                } else {
                    ++frames;
                }
                if (phx_release_buffer(cam) != 0) {
                    goto error;
                }
                if (frames >= count) {
                    goto stop;
                }
            }
            previous = index;
        }

        fprintf(stdout, "Processed frames: %10ld\n", frames);
        fprintf(stdout, "Errors:           %10ld\n", errors);
        fprintf(stdout, "Timeouts:         %10ld\n", timeouts);
        fprintf(stdout, "Overflows:        %10ld\n", (long)cam->overflows);
        fprintf(stdout, "Lost frames:      %10ld\n", (long)cam->lostframes);
        fprintf(stdout, "Lost synchronizations: %5ld\n", (long)cam->lostsyncs);

        if (phx_any_errors(cam)) {
            phx_report_errors(cam);
        }
    }

    phx_destroy(cam);
    return 0;
}
