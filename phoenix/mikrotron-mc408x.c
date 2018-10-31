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

#define USE_FRAMEGRABBER_FOR_SETTING_CONNECTION 1

#define ROUND_DOWN(a, b)  (((a)/(b))*(b))

#define ROUND_UP(a, b)  ((((b) - 1 + (a))/(b))*(b))

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
    if ((xsub != req_xsub &&
         cxp_set(cam, DECIMATION_HORIZONTAL, req_xsub) != 0) ||
        (ysub != req_ysub &&
         cxp_set(cam, DECIMATION_VERTICAL,   req_ysub) != 0)) {
        return -1;
    }
    return 0;
}

static int
set_pattern_noise_reduction(phx_camera_t* cam, int flag)
{
    uint32_t val, req_val = (flag ? 1 : 0);
    if (cxp_get(cam, FIXED_PATTERN_NOISE_REDUCTION, &val) != 0 ||
        (val != req_val &&
         cxp_set(cam, FIXED_PATTERN_NOISE_REDUCTION, req_val) != 0)) {
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


/*
 * set values of parameters `PHX_CAM_SRC_COL`, `PHX_CAM_SRC_DEPTH` and
 * `PHX_DST_FORMAT` corresponding to the camera pixel format which is one of
 * `PIXEL_FORMAT_MONO8`, `PIXEL_FORMAT_MONO10`, `PIXEL_FORMAT_BAYERGR8` or
 * `PIXEL_FORMAT_BAYERGR10`.
 */

static int
set_other_formats(phx_camera_t* cam)
{
    switch (cam->pixel_format) {
    case CXP_PIXEL_FORMAT_MONO8:
        cam->cam_color = PHX_CAM_SRC_MONO;
        cam->buf_format = PHX_DST_FORMAT_Y8;
        return 0;
    case CXP_PIXEL_FORMAT_MONO10:
        cam->cam_color = PHX_CAM_SRC_MONO;
        cam->buf_format = PHX_DST_FORMAT_Y16;
        return 0;
    case CXP_PIXEL_FORMAT_BAYERGR8:
        cam->cam_color = PHX_CAM_SRC_BAY_RGGB;
        cam->buf_format = PHX_DST_FORMAT_BAY8;
        return 0;
    case CXP_PIXEL_FORMAT_BAYERGR10:
        cam->cam_color = PHX_CAM_SRC_BAY_RGGB;
        cam->buf_format = PHX_DST_FORMAT_BAY16;
        return 0;
    default:
        tao_push_error(&cam->errs, __func__, TAO_BAD_TYPE);
        return -1;
    }
}


/*
 * Update pixel format and set bits per pixels.
 */

static int
update_pixel_format(phx_camera_t* cam)
{
    uint32_t val;
    if (cxp_get(cam, PIXEL_FORMAT, &val) != 0) {
        return -1;
    }
    cam->pixel_format = val;
    if (val == CXP_PIXEL_FORMAT_MONO8 ||
        val == CXP_PIXEL_FORMAT_BAYERGR8) {
        cam->dev_cfg.depth = 8;
    } else if (val == CXP_PIXEL_FORMAT_MONO10 ||
               val == CXP_PIXEL_FORMAT_BAYERGR10) {
        cam->dev_cfg.depth = 10;
    } else {
        tao_push_error(&cam->errs, __func__, TAO_BAD_TYPE);
        return -1;
    }
    return set_other_formats(cam);
}

static int
set_bits_per_pixel(phx_camera_t* cam, int depth)
{
    uint32_t pixel_format = 0;
    if (cam->pixel_format == CXP_PIXEL_FORMAT_MONO8 ||
        cam->pixel_format == CXP_PIXEL_FORMAT_MONO10) {
        if (depth == 8) {
            pixel_format = CXP_PIXEL_FORMAT_MONO8;
        } else if (depth == 10) {
            pixel_format = CXP_PIXEL_FORMAT_MONO10;
        } else {
            goto error;
        }
    } else if (cam->pixel_format == CXP_PIXEL_FORMAT_BAYERGR8 ||
               cam->pixel_format == CXP_PIXEL_FORMAT_BAYERGR10) {
        if (depth == 8) {
            pixel_format = CXP_PIXEL_FORMAT_BAYERGR8;
        } else if (depth == 10) {
            pixel_format = CXP_PIXEL_FORMAT_BAYERGR10;
        } else {
            goto error;
        }
    } else {
        tao_push_error(&cam->errs, __func__, TAO_BAD_TYPE);
        return -1;
    }
    if (cam->pixel_format != pixel_format) {
        if (cxp_set(cam, PIXEL_FORMAT, pixel_format) != 0) {
            return -1;
        }
        cam->pixel_format = pixel_format;
        cam->dev_cfg.depth = depth;
    }
    return set_other_formats(cam);

 error:
    tao_push_error(&cam->errs, __func__, TAO_BAD_DEPTH);
    return -1;
}

static int
update_connection(phx_camera_t* cam)
{
    const uint32_t msk = (CXP_CONNECTION_CONFIG_CONNECTION_1 |
                          CXP_CONNECTION_CONFIG_CONNECTION_2 |
                          CXP_CONNECTION_CONFIG_CONNECTION_3 |
                          CXP_CONNECTION_CONFIG_CONNECTION_4);
    uint32_t val;
    if (cxp_get(cam, CONNECTION_CONFIG, &val) != 0) {
        return -1;
    }
    switch (val & msk) {
    case CXP_CONNECTION_CONFIG_CONNECTION_1:
        cam->dev_cfg.connection.channels = 1;
        break;
    case CXP_CONNECTION_CONFIG_CONNECTION_2:
        cam->dev_cfg.connection.channels = 2;
        break;
    case CXP_CONNECTION_CONFIG_CONNECTION_3:
        cam->dev_cfg.connection.channels = 3;
        break;
    case CXP_CONNECTION_CONFIG_CONNECTION_4:
        cam->dev_cfg.connection.channels = 4;
        break;
    default:
        cam->dev_cfg.connection.channels = 0;
    }
    switch (val & ~msk) {
    case CXP_CONNECTION_CONFIG_SPEED_1250:
        cam->dev_cfg.connection.speed = 1250;
        break;
    case CXP_CONNECTION_CONFIG_SPEED_2500:
        cam->dev_cfg.connection.speed = 2500;
        break;
    case CXP_CONNECTION_CONFIG_SPEED_3125:
        cam->dev_cfg.connection.speed = 3125;
        break;
    case CXP_CONNECTION_CONFIG_SPEED_5000:
        cam->dev_cfg.connection.speed = 5000;
        break;
    case CXP_CONNECTION_CONFIG_SPEED_6250:
        cam->dev_cfg.connection.speed = 6250;
        break;
    default:
        cam->dev_cfg.connection.speed = 0;
    }
    return 0;
}

static int
set_connection(phx_camera_t* cam, const phx_connection_t* con)
{
#if USE_FRAMEGRABBER_FOR_SETTING_CONNECTION
    if (phx_set_coaxpress_connection(cam, con) != 0) {
        return -1;
    }
    if (update_connection(cam) != 0) {
        return -1;
    }
    if (con->channels != 0 &&
        con->channels != cam->dev_cfg.connection.channels) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_CHANNELS);
    }
    if (con->speed != 0 &&
        con->speed != cam->dev_cfg.connection.speed) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_SPEED);
    }
#else
    uint32_t val;
    switch (con->channels) {
    case 1:
        val = CXP_CONNECTION_CONFIG_CONNECTION_1;
        break;
    case 2:
        val = CXP_CONNECTION_CONFIG_CONNECTION_2;
        break;
    case 3:
        val = CXP_CONNECTION_CONFIG_CONNECTION_3;
        break;
    case 4:
        val = CXP_CONNECTION_CONFIG_CONNECTION_4;
        break;
    default:
        tao_push_error(&cam->errs, __func__, TAO_BAD_CHANNELS);
        return -1;
    }
    switch (con->speed) {
    case 1250:
        val |= CXP_CONNECTION_CONFIG_SPEED_1250;
        break;
    case 2500:
        val |= CXP_CONNECTION_CONFIG_SPEED_2500;
        break;
    case 3125:
        val |= CXP_CONNECTION_CONFIG_SPEED_3125;
        break;
    case 5000:
        val |= CXP_CONNECTION_CONFIG_SPEED_5000;
        break;
    case 6250:
        val |= CXP_CONNECTION_CONFIG_SPEED_6250;
        break;
        tao_push_error(&cam->errs, __func__, TAO_BAD_SPEED);
        return -1;
    }
    if (cxp_set(cam, CONNECTION_CONFIG, val) != 0) {
        return -1;
    }
    cam->dev_cfg.connection.channels = con->channels;
    cam->dev_cfg.connection.speed = con->speed;
#endif
    return 0;
}

/*
 * Update and set detetctor bias (black level).
 */

static int
update_black_level(phx_camera_t* cam)
{
    uint32_t val;
    if (cxp_get(cam, BLACK_LEVEL, &val) != 0) {
        return -1;
    }
    cam->dev_cfg.bias = (double)val;
    return 0;
}

static int
set_black_level(phx_camera_t* cam, double arg)
{
    if (isnan(arg) ||
        arg <= (cxp_min(BLACK_LEVEL) - 0.5) ||
        arg >= (cxp_max(BLACK_LEVEL) + 0.5)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
    }
    uint32_t val = lround(arg);
    if (cxp_set(cam, BLACK_LEVEL, val) != 0) {
        return -1;
    }
    cam->dev_cfg.bias = (double)val;
    return 0;
}


/*
 * Update and set detector gain.
 */

static int
update_gain(phx_camera_t* cam)
{
    uint32_t val;
    if (cxp_get(cam, GAIN, &val) != 0) {
        return -1;
    }
    cam->dev_cfg.gain = (double)val;
    return 0;
}

static int
set_gain(phx_camera_t* cam, double arg)
{
    if (isnan(arg) ||
        arg <= (cxp_min(GAIN) - 0.5) ||
        arg >= (cxp_max(GAIN) + 0.5)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
    }
    uint32_t val = lround(arg);
    if (cxp_set(cam, GAIN, val) != 0) {
        return -1;
    }
    cam->dev_cfg.gain = (double)val;
    return 0;
}


/*
 * Update and set exposure time (device value is in µs).
 */

static int
update_exposure_time(phx_camera_t* cam)
{
    uint32_t val;
    if (cxp_get(cam, EXPOSURE_TIME, &val) != 0) {
        return -1;
    }
    cam->dev_cfg.exposure = 1e-6*val;
    return 0;
}

static int
set_exposure_time(phx_camera_t* cam, double arg)
{
    if (isnan(arg) ||
        arg <= 1e-6*(cxp_min(EXPOSURE_TIME) - 0.5) ||
        arg >= 1e-6*(cxp_max(EXPOSURE_TIME) + 0.5)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
    }
    uint32_t val = lround(arg*1e6);
    if (cxp_set(cam, EXPOSURE_TIME, val) != 0) {
        return -1;
    }
    cam->dev_cfg.exposure = 1e-6*val;
    return 0;
}


/*
 * Update and set acquisition frame rate (device value is in Hz).
 */

static int
update_frame_rate(phx_camera_t* cam)
{
    uint32_t val;
    if (cxp_get(cam, ACQUISITION_FRAME_RATE, &val) != 0) {
        return -1;
    }
    cam->dev_cfg.rate = (double)val;
    return 0;
}

static int
set_frame_rate(phx_camera_t* cam, double arg)
{
    if (isnan(arg) ||
        arg <= (cxp_min(ACQUISITION_FRAME_RATE) - 0.5) ||
        arg >= (cxp_max(ACQUISITION_FRAME_RATE) + 0.5)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
    }
    uint32_t val = lround(arg);
    if (cxp_set(cam, ACQUISITION_FRAME_RATE, val) != 0) {
        return -1;
    }
    cam->dev_cfg.rate = (double)val;
    return 0;
}


/*
 * Update and set region of interest.
 */

static int
update_region_of_interest(phx_camera_t* cam)
{
    uint32_t xoff, yoff, width, height;
    if (cxp_get(cam, OFFSET_X, &xoff) != 0 ||
        cxp_get(cam, OFFSET_Y, &yoff) != 0 ||
        cxp_get(cam, WIDTH,   &width) != 0 ||
        cxp_get(cam, HEIGHT, &height) != 0) {
        return -1;
    }
    cam->dev_cfg.roi.xoff = xoff;
    cam->dev_cfg.roi.yoff = yoff;
    cam->dev_cfg.roi.width = width;
    cam->dev_cfg.roi.height = height;
    return 0;
}

static int
set_region_of_interest(phx_camera_t* cam, const phx_roi_t* arg)
{
    if (arg->xoff < 0 || arg->width < 1 ||
        arg->yoff < 0 || arg->height < 1 ||
        arg->xoff + arg->width > cam->fullwidth ||
        arg->yoff + arg->height > cam->fullheight) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ROI);
    }
    phx_roi_t newroi;
    newroi.xoff = ROUND_DOWN(arg->xoff, CXP_HORIZONTAL_INCREMENT);
    newroi.yoff = ROUND_DOWN(arg->yoff, CXP_VERTICAL_INCREMENT);
    newroi.width = ROUND_UP(arg->xoff + arg->width,
                            CXP_HORIZONTAL_INCREMENT) - newroi.xoff;
    newroi.height = ROUND_UP(arg->yoff + arg->height,
                             CXP_VERTICAL_INCREMENT) - newroi.yoff;
    phx_roi_t* oldroi = &cam->dev_cfg.roi;
#define CFG_SET(m, id)                              \
        if (cxp_set(cam, id, newroi.m) != 0) {      \
            return -1;                              \
        }                                           \
        oldroi->m = newroi.m
    if (newroi.xoff < oldroi->xoff) {
        CFG_SET(xoff, OFFSET_X);
    }
    if (newroi.width != oldroi->width) {
        CFG_SET(width, WIDTH);
    }
    if (newroi.xoff > oldroi->xoff) {
        CFG_SET(xoff, OFFSET_X);
    }
    if (newroi.yoff < oldroi->yoff) {
        CFG_SET(yoff, OFFSET_Y);
    }
    if (newroi.height != oldroi->height) {
        CFG_SET(height, HEIGHT);
    }
    if (newroi.yoff > oldroi->yoff) {
        CFG_SET(yoff, OFFSET_Y);
    }
#undef CFG_SET
    return 0;
}


/*
 * Update camera configuration according to current device settings.
 */

static int
update_config(phx_camera_t* cam)
{
    if (update_connection(        cam) != 0 ||
        update_pixel_format(      cam) != 0 ||
        update_region_of_interest(cam) != 0 ||
        update_black_level(       cam) != 0 ||
        update_gain(              cam) != 0 ||
        update_exposure_time(     cam) != 0 ||
        update_frame_rate(        cam) != 0 ||
        update_temperature(       cam) != 0) {
        return -1;
    }
    return 0;
}

/*
 * When changing the configuration, we attempt to only change the settings that
 * need to be changed and in an order such as to avoid clashes. The strategy
 * is:
 *
 * 1. reduce bits per pixel if requested;
 * 2. reduce frame rate if requested;
 * 3. reduce exposure time if requested;
 * 4. change ROI if requested;
 * 5. augment exposure time if requested;
 * 6. augment frame rate if requested;
 * 7. augment bits per pixel if requested;
 */
static int
set_config(phx_camera_t* cam)
{
    phx_config_t* dev = &cam->dev_cfg;
    phx_config_t* usr = &cam->usr_cfg;

    /* Check configuration parameters. */
    if (isnan(usr->bias)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_BIAS);
    }
    if (isnan(usr->gain)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_GAIN);
    }
    if (isnan(usr->exposure)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_EXPOSURE);
    }
    if (isnan(usr->rate)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_RATE);
    }
    if (usr->connection.channels < 1 || usr->connection.channels > 4 ||
        usr->connection.speed < 1250 || usr->connection.speed > 6250) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_SPEED);
    }

    /* Change black level and gain if requested. */
    if (usr->bias != dev->bias) {
        if (set_black_level(cam, usr->bias) != 0) {
            return -1;
        }
        usr->bias = dev->bias;
    }
    if (usr->gain != dev->gain) {
        if (set_gain(cam, usr->gain) != 0) {
            return -1;
        }
        usr->gain = dev->gain;
    }

    /* Augment connection speed if requested. */
    if (usr->connection.channels*usr->connection.speed >
        dev->connection.channels*dev->connection.speed) {
        if (set_connection(cam, &usr->connection) != 0) {
            return -1;
        }
        dev->connection.channels = usr->connection.channels;
        dev->connection.speed = usr->connection.speed;
    }

    /* Reduce bits per pixel if requested. */
    if (usr->depth < dev->depth) {
        if (set_bits_per_pixel(cam, usr->depth) != 0) {
            return -1;
        }
        usr->depth = dev->depth;
    }

    /* Reduce frame rate if requested. */
    if (usr->rate < dev->rate) {
        if (set_frame_rate(cam, usr->rate) != 0) {
            return -1;
        }
        usr->rate = dev->rate;
    }

    /* Reduce exposure time if requested. */
    if (usr->exposure < dev->exposure) {
        if (set_exposure_time(cam, usr->exposure) != 0) {
            return -1;
        }
        usr->exposure = dev->exposure;
    }

    /* Change the ROI if it has to change. */
    if (set_region_of_interest(cam, &usr->roi) != 0) {
        return -1;
    }

    /* Augment exposure time if requested. */
    if (usr->exposure > dev->exposure) {
        if (set_exposure_time(cam, usr->exposure) != 0) {
            return -1;
        }
        usr->exposure = dev->exposure;
    }

    /* Augment frame rate if requested. */
    if (usr->rate > dev->rate) {
        if (set_frame_rate(cam, usr->rate) != 0) {
            return -1;
        }
        usr->rate = dev->rate;
    }

    /* Augment bits per pixel if requested. */
    if (usr->depth > dev->depth) {
        if (set_bits_per_pixel(cam, usr->depth) != 0) {
            return -1;
        }
        usr->depth = dev->depth;
    }

    /* Reduce connection speed if requested. */
    if (usr->connection.channels != dev->connection.channels ||
        usr->connection.speed  != dev->connection.speed) {
        if (set_connection(cam, &usr->connection) != 0) {
            return -1;
        }
        dev->connection.channels = usr->connection.channels;
        dev->connection.speed = usr->connection.speed;
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
     * Get sensor size.
     */
    uint32_t fullwidth, fullheight;
    if (cxp_get(cam, SENSOR_WIDTH,   &fullwidth) != 0 ||
        cxp_get(cam, SENSOR_HEIGHT, &fullheight) != 0) {
        return -1;
    }
    cam->fullwidth = fullwidth;
    cam->fullheight = fullheight;

    /*
     * Update other device information.
     */
    if (update_config(cam) != 0) {
        return -1;
    }

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
     * Set acquisition parameters (FIXME: some are re-set in phx_start).
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
     * Set hooks.
     */
    cam->start = start;
    cam->stop = stop;
    cam->update_temperature = update_temperature;
    cam->set_config = set_config;

    /*
     * Make user settings identical to that of the device.
     */
    memcpy(&cam->usr_cfg, &cam->dev_cfg, sizeof(cam->dev_cfg));

    return 0;
}
