/*
 * phoenix-types.h --
 *
 * Type definitions for high level interface to ActiveSilicon Phoenix frame
 * grabber.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2016, Éric Thiébaut & Jonathan Léger.
 * Copyright (C) 2017-2018, Éric Thiébaut.
 */

#ifndef _PHOENIX_TYPES_H
#define _PHOENIX_TYPES_H 1

#include <stdint.h>
#include <phx_api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef etStat        phx_status_t;
typedef etParam       phx_param_t;
typedef etParamValue  phx_value_t;
typedef etBoardInfo   phx_board_info_t;
typedef tHandle       phx_handle_t;
typedef stImageBuff   phx_imgbuf_t;
typedef tPHX          phx_pointer_t;
typedef etAcq         phx_acquisition_t;
typedef etControlPort phx_control_port_t;

typedef float        float32_t;
typedef double       float64_t;

#ifdef __cplusplus
}
#endif

#endif /* _PHOENIX_TYPES_H */
