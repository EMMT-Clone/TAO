/*
 * phx_config.c -
 *
 * Program to configure a camera connected to an ActiveSilicon Phoenix frame
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

static const char* progname = "phx_config";

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
                        "Test configuration of the camera.\n");
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
        printf("Board information:\n");
        phx_print_board_info(cam, "  ", stdout);
        printf("Connection channels: %u\n", cam->dev_cfg.connection.channels);
        printf("Connection speed:    %u Mbps\n", cam->dev_cfg.connection.speed);
        printf("Camera vendor: %s\n",
               (cam->vendor[0] != '\0' ? cam->vendor : "Unknown"));
        printf("Camera model: %s\n",
               (cam->model[0] != '\0' ? cam->model : "Unknown"));
        printf("CoaXPress camera: %s\n",
               (cam->coaxpress ? "yes" : "no"));
        printf("Detector bias: %5.1f\n", cam->dev_cfg.bias);
        printf("Detector gain: %5.1f\n", cam->dev_cfg.gain);
        printf("Exposure time: %g s\n", cam->dev_cfg.exposure);
        printf("Frame rate: %.1f Hz\n", cam->dev_cfg.rate);
        printf("Bits per pixel: %d\n", (int)cam->dev_cfg.depth);
        printf("Sensor size: %d × %d pixels\n",
               (int)cam->fullwidth,  (int)cam->fullheight);
        printf("Region of interest: %d × %d at (%d,%d)\n",
               (int)cam->usr_cfg.roi.width,  (int)cam->usr_cfg.roi.height,
               (int)cam->usr_cfg.roi.xoff, (int)cam->usr_cfg.roi.yoff);
        printf("Active region:      %d × %d at (%d,%d)\n",
               (int)cam->dev_cfg.roi.width,  (int)cam->dev_cfg.roi.height,
               (int)cam->dev_cfg.roi.xoff, (int)cam->dev_cfg.roi.yoff);
        if (cam->update_temperature != NULL) {
            printf("Detector temperature: %.1f °C\n", cam->temperature);
        }
    }

    phx_destroy(cam);
    return 0;
}
