/*
 * andor-convert.c --
 *
 * Routines for pixel encoding conversions with Andor cameras library.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#ifndef _ANDOR_CONVERT_C
#define _ANDOR_CONVERT_C 1

#include "andor.h"

/* SRC_TYPE is uint8_t */
#define SRC_TYPE uint8_t
# define DST_TYPE uint8_t
# define CONVERT_MONO convert_u8_to_u8
# include __FILE__
# define DST_TYPE uint16_t
# define CONVERT_MONO convert_u8_to_u16
# include __FILE__
# define DST_TYPE uint32_t
# define CONVERT_MONO convert_u8_to_u32
# include __FILE__
# define DST_TYPE float
# define CONVERT_MONO convert_u8_to_f32
# include __FILE__
# define DST_TYPE double
# define CONVERT_MONO convert_u8_to_f64
# include __FILE__
#undef SRC_TYPE

#define SRC_TYPE uint16_t
# define DST_TYPE uint8_t
# define CONVERT_MONO convert_u16_to_u8
# include __FILE__
# define DST_TYPE uint16_t
# define CONVERT_MONO convert_u16_to_u16
# include __FILE__
# define DST_TYPE uint32_t
# define CONVERT_MONO convert_u16_to_u32
# include __FILE__
# define DST_TYPE float
# define CONVERT_MONO convert_u16_to_f32
# include __FILE__
# define DST_TYPE double
# define CONVERT_MONO convert_u16_to_f64
# include __FILE__
#undef SRC_TYPE

#define SRC_TYPE uint32_t
# define DST_TYPE uint8_t
# define CONVERT_MONO convert_u32_to_u8
# include __FILE__
# define DST_TYPE uint16_t
# define CONVERT_MONO convert_u32_to_u16
# include __FILE__
# define DST_TYPE uint32_t
# define CONVERT_MONO convert_u32_to_u32
# include __FILE__
# define DST_TYPE float
# define CONVERT_MONO convert_u32_to_f32
# include __FILE__
# define DST_TYPE double
# define CONVERT_MONO convert_u32_to_f64
# include __FILE__
#undef SRC_TYPE

#define TMP_TYPE uint16_t /* FIXME: faster with uint32_t in some cases? */
# define DST_TYPE uint8_t
# define CONVERT_MONO12PACKED convert_m12p_to_u8
# include __FILE__
# define DST_TYPE uint16_t
# define CONVERT_MONO12PACKED convert_m12p_to_u16
# include __FILE__
# define DST_TYPE uint32_t
# define CONVERT_MONO12PACKED convert_m12p_to_u32
# include __FILE__
# define DST_TYPE float
# define CONVERT_MONO12PACKED convert_m12p_to_f32
# include __FILE__
# define DST_TYPE double
# define CONVERT_MONO12PACKED convert_m12p_to_f64
# include __FILE__
#undef TMP_TYPE

int
andor_convert_buffer(void* dst, andor_encoding_t dst_enc,
                     const void* src,  andor_encoding_t src_enc,
                     long width, long height, long stride)
{
    void (*convert)(uint8_t*, const uint8_t*, long, long, long);
    switch (src_enc) {
    case ANDOR_ENCODING_MONO8:
        switch (dst_enc) {
        case ANDOR_ENCODING_MONO8:
            convert = convert_u8_to_u8;
            break;
        case ANDOR_ENCODING_MONO16:
            convert = convert_u8_to_u16;
            break;
        case ANDOR_ENCODING_MONO32:
            convert = convert_u8_to_u32;
            break;
        case ANDOR_ENCODING_FLOAT:
            convert = convert_u8_to_f32;
            break;
        case ANDOR_ENCODING_DOUBLE:
            convert = convert_u8_to_f64;
            break;
        default:
            convert = NULL;
        }
        break;
    case ANDOR_ENCODING_MONO12:
    case ANDOR_ENCODING_MONO16:
        switch (dst_enc) {
        case ANDOR_ENCODING_MONO8:
            convert = convert_u16_to_u8;
            break;
        case ANDOR_ENCODING_MONO16:
            convert = convert_u16_to_u16;
            break;
        case ANDOR_ENCODING_MONO32:
            convert = convert_u16_to_u32;
            break;
        case ANDOR_ENCODING_FLOAT:
            convert = convert_u16_to_f32;
            break;
        case ANDOR_ENCODING_DOUBLE:
            convert = convert_u16_to_f64;
            break;
        default:
            convert = NULL;
        }
        break;
    case ANDOR_ENCODING_MONO32:
        switch (dst_enc) {
        case ANDOR_ENCODING_MONO8:
            convert = convert_u32_to_u8;
            break;
        case ANDOR_ENCODING_MONO16:
            convert = convert_u32_to_u16;
            break;
        case ANDOR_ENCODING_MONO32:
            convert = convert_u32_to_u32;
            break;
        case ANDOR_ENCODING_FLOAT:
            convert = convert_u32_to_f32;
            break;
        case ANDOR_ENCODING_DOUBLE:
            convert = convert_u32_to_f64;
            break;
        default:
            convert = NULL;
        }
        break;
    case ANDOR_ENCODING_MONO12PACKED:
        switch (dst_enc) {
        case ANDOR_ENCODING_MONO8:
            convert = convert_m12p_to_u8;
            break;
        case ANDOR_ENCODING_MONO16:
            convert = convert_m12p_to_u16;
            break;
        case ANDOR_ENCODING_MONO32:
            convert = convert_m12p_to_u32;
            break;
        case ANDOR_ENCODING_FLOAT:
            convert = convert_m12p_to_f32;
            break;
        case ANDOR_ENCODING_DOUBLE:
            convert = convert_m12p_to_f64;
            break;
        default:
            convert = NULL;
        }
        break;
    default:
        convert = NULL;
    }
    if (convert == NULL) {
        return -1;
    }
    convert(dst, src, width, height, stride);
    return 0;
}

#else /* _ANDOR_CONVERT_C */

#ifdef CONVERT_MONO
static void
CONVERT_MONO(uint8_t* dstdata, const uint8_t* srcdata,
             long width, long height, long stride)
{
    for (long y = 0; y < height; ++y) {
        DST_TYPE* dst = ((DST_TYPE*)dstdata) + y*width;
        const SRC_TYPE* src = (const SRC_TYPE*)(srcdata + y*stride);
        for (long x = 0; x < width; ++x) {
            dst[x] = src[x];
        }
    }
}
#undef CONVERT_MONO
#endif

#ifdef CONVERT_MONO12PACKED
static void
CONVERT_MONO12PACKED(uint8_t* dstdata, const uint8_t* srcdata,
                     long width, long height, long stride)
{
    TMP_TYPE b0, b1, b2;
    long xm = width - 1;
    bool isodd = ((width&1) != 0);

    for (long y = 0; y < height; ++y) {
        DST_TYPE* dst = ((DST_TYPE*)dstdata) + y*width;
        const uint8_t* src = srcdata + y*stride;
        for (long x = 0; x < xm; x += 2) {
            b0 = src[0];
            b1 = src[1];
            b2 = src[2];
            src += 3;
            dst[x]   = (b0 << 4) | (b1 & (TMP_TYPE)0x000F);
            dst[x+1] = (b2 << 4) | (b1 >> 4);
        }
        if (isodd) {
            b0 = src[0];
            b1 = src[1];
            dst[xm] = (b0 << 4) | (b1 & (TMP_TYPE)0x000F);
        }
    }
}
#undef CONVERT_MONO12PACKED
#endif

#undef DST_TYPE

#endif /* _ANDOR_CONVERT_C */
