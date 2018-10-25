/*
 * phx_info.c --
 *
 * Simple program to connect to a camera via ActiveSilicon Phoenix frame
 * grabber.
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
  printf("Camera vendor: %s\n",
         (cam->vendor[0] != '\0' ? cam->vendor : "Unknown"));
  printf("Camera model: %s\n",
         (cam->model[0] != '\0' ? cam->model : "Unknown"));
  printf("CoaXPress camera: %s\n",
         (cam->coaxpress ? "yes" : "no"));
  printf("Sensor size: %d × %d pixels\n",
         (int)cam->fullwidth,  (int)cam->fullheight);
  printf("Region of interest: %d × %d at (%d,%d)\n",
         (int)cam->width,  (int)cam->height,
         (int)cam->xoff, (int)cam->yoff);
#if 0
  printf("Detector bias: %.3f\n", cam->bias);
  printf("Detector gain: %.3f\n", cam->gain);
#endif
  if (cam->update_temperature != NULL) {
      printf("Detector temperature: %.1f °C\n", cam->temperature);
  }
  phx_destroy(cam);
  return 0;
}
