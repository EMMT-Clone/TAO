/*
 * phx_test.c -
 *
 * Simple program to test acquisition with ActiveSilicon Phoenix frame grabber.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <stdio.h>

#include "phoenix.h"

static int
parse_options(int argc, char* argv[], phx_config_t* cfg)
{
    const char* progname = argv[0];
    char c;
    char buffer[30];

    /* Parse arguments. */
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg[0] != '-') {
            return i;
        }
        if (strcmp(arg, "--") == 0) {
            return i + 1;
        }
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            fprintf(stderr, "Usage: %s [OPTIONS] [--] OUTPUT\n", progname);
            fprintf(stderr,
                    "Test configuration of the camera.\n");
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "  --roi XOFF,YOFF,WIDTH,HEIGHT  "
                    "Region of interest [");
            fprintf(stderr, "%d,%d,%d,%d",
                    cfg->roi.xoff, cfg->roi.yoff,
                    cfg->roi.width, cfg->roi.height);
            fputs("].\n", stderr);
            fprintf(stderr, "  --depth BITS                  "
                    "Bits per pixel [%d bits].\n", cfg->depth);
            fprintf(stderr, "  --rate FPS                    "
                    "Frames per second [%g Hz].\n", cfg->rate);
            fprintf(stderr, "  --exposure TIME               "
                    "Exposure duration in seconds [%g s].\n", cfg->exposure);
            fprintf(stderr, "  --bias LEVEL                  "
                    "Black level [%g].\n", cfg->bias);
            fprintf(stderr, "  --gain VALUE                  "
                    "Gain [%g].\n", cfg->gain);
            if (cfg->connection.speed != 0) {
                sprintf(buffer, "%u Mbps",
                        (unsigned int)cfg->connection.speed);
            } else {
                strcpy(buffer, "auto");
            }
            fprintf(stderr, "  --bitrate VALUE               "
                    "CoaXPress bitrate [%s].\n", buffer);
            if (cfg->connection.channels != 0) {
                sprintf(buffer, "%u", (unsigned int)cfg->connection.channels);
            } else {
                strcpy(buffer, "auto");
            }
            fprintf(stderr, "  --channels NUMBER             "
                    "Number of CoaXPress channels [%s].\n", buffer);
            fprintf(stderr, "  --quiet                       "
                    "Quiet (non-verbose) mode.\n");
            fprintf(stderr, "  -h, --help                    "
                    "Print this help.\n");
            exit(0);
        } else if (strcmp(arg, "--roi") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "%s: missing argument to option `--roi`\n",
                        progname);
                exit(1);
            }
            if (sscanf(argv[i], " %d , %d , %d , %d %c",
                       &cfg->roi.xoff, &cfg->roi.yoff,
                       &cfg->roi.width, &cfg->roi.height, &c) != 4) {
                fprintf(stderr, "%s: bad argument for option `--roi`, "
                        "should be `XOFF,YOFF,WIDTH,HEIGHT`\n", progname);
                exit(1);
            }
        } else if (strcmp(arg, "--depth") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "%s: missing argument to option `--depth`\n",
                        progname);
                exit(1);
            }
            if (sscanf(argv[i], " %d %c", &cfg->depth, &c) != 1) {
                fprintf(stderr, "%s: bad argument for option `--depth`, "
                        "should be `FPS`\n", progname);
                exit(1);
            }
            if (cfg->depth < 1) {
                fprintf(stderr, "%s: bad value for option `--depth`\n",
                        progname);
                exit(1);
            }
        } else if (strcmp(arg, "--rate") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "%s: missing argument to option `--rate`\n",
                        progname);
                exit(1);
            }
            if (sscanf(argv[i], " %lf %c", &cfg->rate, &c) != 1) {
                fprintf(stderr, "%s: bad argument for option `--rate`, "
                        "should be `FPS`\n", progname);
                exit(1);
            }
            if (cfg->rate < 1 || cfg->rate > INT32_MAX) {
                fprintf(stderr, "%s: bad value for option `--rate`\n",
                        progname);
                exit(1);
            }
        } else if (strcmp(arg, "--exposure") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "%s: missing argument to option `--exposure`\n",
                        progname);
                exit(1);
            }
            if (sscanf(argv[i], " %lf %c", &cfg->exposure, &c) != 1) {
                fprintf(stderr, "%s: bad argument for option `--exposure`, "
                        "should be `FPS`\n", progname);
                exit(1);
            }
            if (cfg->exposure < 1 || cfg->exposure > INT32_MAX) {
                fprintf(stderr, "%s: bad value for option `--exposure`\n",
                        progname);
                exit(1);
            }
        } else if (strcmp(arg, "--bias") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "%s: missing argument to option `--bias`\n",
                        progname);
                exit(1);
            }
            if (sscanf(argv[i], " %lf %c", &cfg->bias, &c) != 1) {
                fprintf(stderr, "%s: bad argument for option `--bias`, "
                        "should be `FPS`\n", progname);
                exit(1);
            }
            if (cfg->bias < 0) {
                fprintf(stderr, "%s: bad value for option `--bias`\n",
                        progname);
                exit(1);
            }
        } else if (strcmp(arg, "--gain") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "%s: missing argument to option `--gain`\n",
                        progname);
                exit(1);
            }
            if (sscanf(argv[i], " %lf %c", &cfg->gain, &c) != 1) {
                fprintf(stderr, "%s: bad argument for option `--gain`, "
                        "should be `FPS`\n", progname);
                exit(1);
            }
            if (cfg->gain < 1) {
                fprintf(stderr, "%s: bad value for option `--gain`\n",
                        progname);
                exit(1);
            }
        } else if (strcmp(arg, "--bitrate") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "%s: missing argument to option `--bitrate`\n",
                        progname);
                exit(1);
            }
            if (strcmp(argv[i], "auto") == 0) {
                cfg->connection.speed = 0;
            } else {
                unsigned int bitrate;
                if (sscanf(argv[i], " %u %c", &bitrate, &c) != 1) {
                    fprintf(stderr, "%s: bad argument for option `--bitrate`, "
                            "should be in Mbps\n", progname);
                    exit(1);
                }
                if (bitrate < 1) {
                    fprintf(stderr, "%s: bad value for option `--bitrate`\n",
                            progname);
                    exit(1);
                }
                cfg->connection.speed = bitrate;
            }
        } else if (strcmp(arg, "--channels") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "%s: missing argument to option `--channels`\n",
                        progname);
                exit(1);
            }
            if (strcmp(argv[i], "auto") == 0) {
                cfg->connection.channels = 0;
            } else {
                unsigned int channels;
                if (sscanf(argv[i], " %u %c", &channels, &c) != 1) {
                    fprintf(stderr, "%s: bad argument for option `--channels`, "
                            "should be in Mbps\n", progname);
                    exit(1);
                }
                if (channels < 1 || channels > 4) {
                    fprintf(stderr, "%s: bad value for option `--channels`\n",
                            progname);
                    exit(1);
                }
                cfg->connection.channels = channels;
            }
        } else {
            fprintf(stderr, "%s: unknown option `%s`\n", progname, argv[i]);
            exit(1);
        }
    }
    return argc;
}

int main(int argc, char* argv[])
{
    tao_error_t* errs = TAO_NO_ERRORS;
    phx_camera_t* cam;
    phx_config_t cfg;

    cam = phx_create(&errs, NULL, NULL, PHX_BOARD_NUMBER_AUTO);
    if (cam == NULL) {
        if (errs == TAO_NO_ERRORS) {
            fprintf(stderr, "Failed to create the camera.\n");
        } else {
            tao_report_errors(&errs);
        }
        return 1;
    }
    phx_get_configuration(cam, &cfg);
    parse_options(argc, argv, &cfg);
    if (phx_set_configuration(cam, &cfg) != 0) {
        if (phx_any_errors(cam)) {
            phx_report_errors(cam);
        } else {
            fprintf(stderr, "Failed to configure the camera.\n");
        }
        return 1;
    }
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

    phx_destroy(cam);
    return 0;
}
