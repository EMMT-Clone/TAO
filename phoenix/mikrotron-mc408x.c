/*
 * mikrotron-mc408x.c -
 *
 * Routines for Mikrotron MC408x cameras.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2017-2018, Éric Thiébaut.
 * Copyright (C) 2016, Éric Thiébaut & Jonathan Léger.
 */

#include "phoenix.h"
#include "coaxpress.h"
#include "mikrotron-mc408x.h"

/* Byte order. */
static const union {
  uint8_t bytes[4];
  uint32_t BOM;
} NATIVE_ENDIAN = {{1, 2, 3, 4}};
#define NATIVE_ENDIAN_BOM (NATIVE_ENDIAN.BOM)
#define LITTLE_ENDIAN_BOM 0x04030201
#define BIG_ENDIAN_BOM    0x01020304

int
phx_check_mikrotron_mc408x(phx_camera_t* cam)
{
    return (cam->coaxpress && strcmp(cam->vendor, "Mikrotron GmbH") == 0 &&
            strncmp(cam->model, "MC408", 5) == 0 &&
            (cam->model[5] == '2' || cam->model[5] == '3' ||
             cam->model[5] == '6' || cam->model[5] == '7'));
}

/*
 * Set the dimensions of the active region for camera `cam`.
 */
static int
set_active_region(phx_camera_t* cam, phx_value_t width, phx_value_t height)
{
    if (phx_set(cam, PHX_CAM_ACTIVE_XOFFSET,      0) != 0 ||
        phx_set(cam, PHX_CAM_ACTIVE_YOFFSET,      0) != 0 ||
        phx_set(cam, PHX_CAM_ACTIVE_XLENGTH,  width) != 0 ||
        phx_set(cam, PHX_CAM_ACTIVE_YLENGTH, height) != 0 ||
        phx_set(cam, PHX_CAM_XBINNING,            1) != 0 ||
        phx_set(cam, PHX_CAM_YBINNING,            1) != 0) {
        return -1;
    }
    return 0;
}

/*    setsourceregion!(cam, xoff, yoff, width, height)
 *
 *set the offsets and dimensions of the *source* region for camera `cam`.  This
 *is a low-level method which does not check its arguments.  It is equivalent to:
 *
 *    cam[PHX_ROI_SRC_XOFFSET] = xoff
 *    cam[PHX_ROI_SRC_YOFFSET] = yoff
 *    cam[PHX_ROI_XLENGTH]     = width
 *    cam[PHX_ROI_YLENGTH]     = height
 *
 *The *source* region is the *capture* region, a.k.a. *ROI*, relative to the
 **active* region.
 *
 *See also: [`setcaptureregion!`](@ref), [`setroi!`](@ref).
 */
static int
set_source_region(phx_camera_t* cam, phx_value_t xoff, phx_value_t yoff,
                  phx_value_t width, phx_value_t height)
{
    if (phx_set(cam, PHX_ROI_SRC_XOFFSET, xoff) != 0 ||
        phx_set(cam, PHX_ROI_SRC_YOFFSET, yoff) != 0 ||
        phx_set(cam, PHX_ROI_XLENGTH,    width) != 0 ||
        phx_set(cam, PHX_ROI_YLENGTH,   height) != 0) {
        return -1;
    }
    return 0;
}

/*
 * Yields values of parameters `PHX_CAM_SRC_COL`, `PHX_CAM_SRC_DEPTH`
 * and `PHX_DST_FORMAT` corresponding to the camera pixel format `pixfmt`
 * which is one of `PIXEL_FORMAT_MONO8`, `PIXEL_FORMAT_MONO10`,
 * `PIXEL_FORMAT_BAYERGR8` or `PIXEL_FORMAT_BAYERGR10`.
 */
static int
get_pixel_format(phx_camera_t* cam, phx_value_t* srcformat,
                 phx_value_t* srcdepth, phx_value_t* dstformat)
{
    uint32_t pixformat;
    if (cxp_get(cam, PIXEL_FORMAT, &pixformat) != 0) {
        return -1;
    }
    if (pixformat == CXP_PIXEL_FORMAT_MONO8) {
        *srcformat = PHX_CAM_SRC_MONO;
        *srcdepth  = 8;
        *dstformat = PHX_DST_FORMAT_Y8;
    } else if (pixformat == CXP_PIXEL_FORMAT_MONO10) {
        *srcformat = PHX_CAM_SRC_MONO;
        *srcdepth  = 10;
        *dstformat = PHX_DST_FORMAT_Y16;
    } else if (pixformat == CXP_PIXEL_FORMAT_BAYERGR8) {
        *srcformat = PHX_CAM_SRC_BAY_RGGB;
        *srcdepth  = 8;
        *dstformat = PHX_DST_FORMAT_BAY8;
    } else if (pixformat == CXP_PIXEL_FORMAT_BAYERGR10) {
        *srcformat = PHX_CAM_SRC_BAY_RGGB;
        *srcdepth  = 10;
        *dstformat = PHX_DST_FORMAT_BAY16;
    } else {
        tao_push_error(&cam->errs, __func__, TAO_BAD_TYPE);
        return -1;
    }
    return 0;
}

static int
start(phx_camera_t* cam)
{
    return cxp_exec(cam, ACQUISITION_START);
}

static int
stop(phx_camera_t* cam)
{
    return cxp_exec(cam, ACQUISITION_STOP);
}

static int
update_temperature(phx_camera_t* cam)
{
    int32_t val; /* must be signed */
    if (cxp_set(cam, DEVICE_INFORMATION_SELECTOR,
                CXP_DEVICE_INFORMATION_SELECTOR_TEMPERATURE) != 0 ||
        cxp_get(cam, DEVICE_INFORMATION, (uint32_t*)&val) != 0) {
        return -1;
    }
    cam->temperature = 0.5*val;
    return 0;
}

static
int set_subsampling(phx_camera_t* cam, int req_xsub, int req_ysub)
{
    uint32_t xsub, ysub;
    if (req_xsub <= 0 || req_ysub <= 0) {
        phx_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
        return -1;
    }
    if (cxp_get(cam, DECIMATION_HORIZONTAL, &xsub) != 0 ||
        cxp_get(cam, DECIMATION_VERTICAL,   &ysub) != 0) {
        return -1;
    }
    if ((xsub != req_xsub && cxp_set(cam, DECIMATION_HORIZONTAL, req_xsub) != 0) ||
        (ysub != req_ysub && cxp_set(cam, DECIMATION_VERTICAL,   req_ysub) != 0)) {
        return -1;
    }
    return 0;
}

static int
set_pattern_noise_reduction(phx_camera_t* cam, int flag)
{
    uint32_t val, req_val = (flag ? 1 : 0);
    if (cxp_get(cam, FIXED_PATTERN_NOISE_REDUCTION, &val) != 0 ||
        (val != req_val && cxp_set(cam, FIXED_PATTERN_NOISE_REDUCTION, req_val) != 0)) {
        return -1;
    }
    return 0;
}

static int
set_filter_mode(phx_camera_t* cam, uint32_t mode)
{
    uint32_t val;
    if (cxp_get(cam, FILTER_MODE, &val) != 0 ||
        (val != mode && cxp_set(cam, FILTER_MODE, mode) != 0)) {
        return -1;
    }
    return 0;
}

static int
set_pixel_pulse_reset(phx_camera_t* cam, int flag)
{
    uint32_t val, req_val = (flag ? 1 : 0);
    if (cxp_get(cam, PRST_ENABLE, &val) != 0 ||
        (val != req_val && cxp_set(cam, PRST_ENABLE, req_val) != 0)) {
        return -1;
    }
    return 0;
}

int
phx_initialize_mikrotron_mc408x(phx_camera_t* cam)
{
    /*
     * Check camera model.
     */
    if (! phx_check_mikrotron_mc408x(cam)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_DEVICE);
        return -1;
    }

    /*
     * Reset sub-sampling.
     */
    if (set_subsampling(cam, 1, 1) != 0) {
        return -1;
    }

    /*
     * Disable fixed pattern noise reduction.
     */
    if (set_pattern_noise_reduction(cam, 0) != 0) {
        return -1;
    }

    /*
     * Set image filter mode to "raw".
     */
    if (set_filter_mode(cam, CXP_FILTER_MODE_RAW) != 0) {
        return -1;
    }

    /*
     * Disable the Pixel Pulse Reset feature (recommanded for frame rates
     * higher or equal to 100 Hz).
     */
    if (set_pixel_pulse_reset(cam, 0) != 0) {
        return -1;
    }

    /*
     * Get current region of interest.
     */
    uint32_t xoff, yoff, width, height, fullwidth, fullheight;
    if (cxp_get(cam, OFFSET_X,            &xoff) != 0 ||
        cxp_get(cam, OFFSET_Y,            &yoff) != 0 ||
        cxp_get(cam, WIDTH,              &width) != 0 ||
        cxp_get(cam, HEIGHT,            &height) != 0 ||
        cxp_get(cam, SENSOR_WIDTH,   &fullwidth) != 0 ||
        cxp_get(cam, SENSOR_HEIGHT, &fullheight) != 0) {
        return -1;
    }
    cam->xoff = xoff;
    cam->yoff = yoff;
    cam->width = width;
    cam->height = height;
    cam->fullwidth = fullwidth;
    cam->fullheight = fullheight;

    /*
     * The following settings are the same as the contents of the configuration
     * file "Mikrotron_MC4080_CXP.pcf".
     */
    if (phx_set(cam, PHX_BOARD_VARIANT,             PHX_DIGITAL) != 0 ||
        phx_set(cam, PHX_CAM_TYPE,         PHX_CAM_AREASCAN_ROI) != 0 ||
        phx_set(cam, PHX_CAM_FORMAT,     PHX_CAM_NON_INTERLACED) != 0 ||
        phx_set(cam, PHX_CAM_CLOCK_POLARITY,  PHX_CAM_CLOCK_POS) != 0 ||
        phx_set(cam, PHX_CAM_SRC_COL,          PHX_CAM_SRC_MONO) != 0 ||
        phx_set(cam, PHX_CAM_DATA_VALID,            PHX_DISABLE) != 0 ||
        phx_set(cam, PHX_CAM_HTAP_NUM,                        1) != 0 ||
        phx_set(cam, PHX_CAM_HTAP_DIR,        PHX_CAM_HTAP_LEFT) != 0 ||
        phx_set(cam, PHX_CAM_HTAP_TYPE,     PHX_CAM_HTAP_LINEAR) != 0 ||
        phx_set(cam, PHX_CAM_HTAP_ORDER, PHX_CAM_HTAP_ASCENDING) != 0 ||
        phx_set(cam, PHX_CAM_VTAP_NUM,                        1) != 0 ||
        phx_set(cam, PHX_CAM_VTAP_DIR,         PHX_CAM_VTAP_TOP) != 0 ||
        phx_set(cam, PHX_CAM_VTAP_TYPE,     PHX_CAM_VTAP_LINEAR) != 0 ||
        phx_set(cam, PHX_CAM_VTAP_ORDER, PHX_CAM_VTAP_ASCENDING) != 0 ||
        phx_set(cam, PHX_COMMS_DATA,           PHX_COMMS_DATA_8) != 0 ||
        phx_set(cam, PHX_COMMS_STOP,           PHX_COMMS_STOP_1) != 0 ||
        phx_set(cam, PHX_COMMS_PARITY,    PHX_COMMS_PARITY_NONE) != 0 ||
        phx_set(cam, PHX_COMMS_SPEED,                      9600) != 0 ||
        phx_set(cam, PHX_COMMS_FLOW,        PHX_COMMS_FLOW_NONE) != 0) {
        return -1;
    }

    /*
     * Set the format of the image sent by the camera.
     */
    phx_value_t srcformat, srcdepth, dstformat;
    if (get_pixel_format(cam, &srcformat, &srcdepth, &dstformat) != 0) {
        return -1;
    }
    cam->srcformat = srcformat;
    cam->srcdepth  = srcdepth;
    cam->dstformat = dstformat;
    if (phx_set(cam, PHX_CAM_SRC_COL,  srcformat) != 0 ||
        phx_set(cam, PHX_CAM_SRC_DEPTH, srcdepth) != 0 ||
        phx_set(cam, PHX_DST_FORMAT,   dstformat) != 0) {
        return -1;
    }
    if (set_active_region(cam, width, height) != 0) {
        return -1;
    }

    /*
     * Set acquisition parameters.
     */
    if (phx_set(cam, PHX_ACQ_BLOCKING,                PHX_ENABLE) != 0 ||
        phx_set(cam, PHX_ACQ_CONTINUOUS,              PHX_ENABLE) != 0 ||
        phx_set(cam, PHX_ACQ_XSUB,                    PHX_ACQ_X1) != 0 ||
        phx_set(cam, PHX_ACQ_YSUB,                    PHX_ACQ_X1) != 0 ||
        phx_set(cam, PHX_ACQ_NUM_BUFFERS,                      1) != 0 ||
        phx_set(cam, PHX_ACQ_IMAGES_PER_BUFFER,                1) != 0 ||
        phx_set(cam, PHX_ACQ_BUFFER_START,                     1) != 0 ||
        phx_set(cam, PHX_DATASTREAM_VALID, PHX_DATASTREAM_ALWAYS) != 0 ||
        phx_set(cam, PHX_TIMEOUT_DMA,    1000 /* milliseconds */) != 0) {
        return -1;
    }

    /*
     * Set source ROI to match the size of the image sent by the camera.
     */
    if (set_source_region(cam, 0, 0, width, height) != 0) {
        return -1;;
    }

    /*
     * Setup destination buffer parameters.  The value of `PHX_BUF_DST_XLENGTH`
     * is the number of bytes per line of the destination buffer (it must be
     * larger of equal the width of the ROI times the number of bits per pixel
     * rounded up to a number of bytes), the value of `PHX_BUF_DST_YLENGTH` is
     * the number of lines in the destination buffer (it must be larger or
     * equal `PHX_ROI_DST_YOFFSET` plus `PHX_ROI_YLENGTH`.
     *
     * FIXME: PHX_BIT_SHIFT_ALIGN_LSB not defined for PHX_BIT_SHIFT
     */
    uint32_t bits = phx_capture_format_bits(dstformat);
    if (phx_set(cam, PHX_ROI_DST_XOFFSET,                  0) != 0 ||
        phx_set(cam, PHX_ROI_DST_YOFFSET,                  0) != 0 ||
        phx_set(cam, PHX_BUF_DST_XLENGTH, (width*bits + 7)/8) != 0 ||
        phx_set(cam, PHX_BUF_DST_YLENGTH,             height) != 0 ||
        phx_set(cam, PHX_BIT_SHIFT,                        0) != 0) {
        return -1;
    }

    /*
     * Use native byte order for the destination buffer.
     */
    if (NATIVE_ENDIAN_BOM == LITTLE_ENDIAN_BOM) {
        if (phx_set(cam, PHX_DST_ENDIAN, PHX_DST_LITTLE_ENDIAN) != 0) {
            return -1;
        }
    } else if (NATIVE_ENDIAN_BOM == BIG_ENDIAN_BOM) {
        if (phx_set(cam, PHX_DST_ENDIAN, PHX_DST_BIG_ENDIAN) != 0) {
            return -1;
        }
    }

    /*
     * Update other information.
     */
    if (update_temperature(cam) != 0) {
        return -1;
    }

    /*
     * Set hooks.
     */
    cam->start = start;
    cam->stop = stop;
    cam->update_temperature = update_temperature;
    return 0;
}
