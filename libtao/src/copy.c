/*
 * copy.c --
 *
 * Copy contents of multi-dimensional arrays in TAO library.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include "config.h"
#include "macros.h"
#include "tao-private.h"


#define TYPE(id) TYPE1(id)
#define TYPE1(id) TYPE__##id

#define TYPE__i8      int8_t
#define TYPE__u8     uint8_t
#define TYPE__i16    int16_t
#define TYPE__u16   uint16_t
#define TYPE__i32    int32_t
#define TYPE__u32   uint32_t
#define TYPE__i64    int64_t
#define TYPE__u64   uint64_t
#define TYPE__f32  float32_t
#define TYPE__f64  float64_t


#define INDEX1(S,j1)             (j1)
#define INDEX2(S,j1,j2)          (j2)*S##dim1 + INDEX1(S,j1)
#define INDEX3(S,j1,j2,j3)       (j3)*S##dim2 + INDEX2(S,j1,j2)
#define INDEX4(S,j1,j2,j3,j4)    (j4)*S##dim3 + INDEX3(S,j1,j2,j3)
#define INDEX5(S,j1,j2,j3,j4,j5) (j5)*S##dim4 + INDEX4(S,j1,j2,j3,j4)

#define OFF1(S)                  INDEX1(S, S##off1)
#define OFF2(S,i2)               INDEX2(S, S##off1,     \
                                        (i2) + S##off2)
#define OFF3(S,i2,i3)            INDEX3(S, S##off1,     \
                                        (i2) + S##off2, \
                                        (i3) + S##off3)
#define OFF4(S,i2,i3,i4)         INDEX4(S, S##off1,     \
                                        (i2) + S##off2, \
                                        (i3) + S##off3, \
                                        (i4) + S##off4)
#define OFF5(S,i2,i3,i4,i5)      INDEX5(S, S##off1,    \
                                        (i2) + S##off2, \
                                        (i3) + S##off3, \
                                        (i4) + S##off4, \
                                        (i5) + S##off5)

#define ENCODE1(PFX, DST, SRC)                                                \
static void                                                                   \
copy_1d_##PFX(void* dstptr, const long dstdims[], const long dstoffs[],       \
              const void* srcptr, const long srcdims[], const long srcoffs[], \
              const long lens[])                                              \
{                                                                             \
    long dstoff1 = dstoffs[0];                                                \
    long srcoff1 = srcoffs[0];                                                \
    long len1 = lens[0];                                                      \
    DST* dst = ((DST*)dstptr) + dstoff1;                                      \
    const SRC* src = ((SRC*)srcptr) + srcoff1;                                \
    for (long i1 = 0; i1 < len1; ++i1) {                                      \
        dst[i1] = src[i1];                                                    \
    }                                                                         \
}

#define ENCODE2(PFX, DST, SRC)                                                \
static void                                                                   \
copy_2d_##PFX(void* dstptr, const long dstdims[], const long dstoffs[],       \
              const void* srcptr, const long srcdims[], const long srcoffs[], \
              const long lens[])                                              \
{                                                                             \
    long dstoff1 = dstoffs[0], dstdim1 = dstdims[0];                          \
    long dstoff2 = dstoffs[1];                                                \
    long srcoff1 = srcoffs[0], srcdim1 = srcdims[0];                          \
    long srcoff2 = srcoffs[1];                                                \
    long len1 = lens[0];                                                      \
    long len2 = lens[1];                                                      \
    for (long i2 = 0; i2 < len2; ++i2) {                                      \
        DST* dst = ((DST*)dstptr) + OFF2(dst,i2);                             \
        const SRC* src = ((SRC*)srcptr) + OFF2(src,i2);                       \
        for (long i1 = 0; i1 < len1; ++i1) {                                  \
            dst[i1] = src[i1];                                                \
        }                                                                     \
    }                                                                         \
}

#define ENCODE3(PFX, DST, SRC)                                                \
static void                                                                   \
copy_3d_##PFX(void* dstptr, const long dstdims[], const long dstoffs[],       \
              const void* srcptr, const long srcdims[], const long srcoffs[], \
              const long lens[])                                              \
{                                                                             \
    long dstoff1 = dstoffs[0], dstdim1 = dstdims[0];                          \
    long dstoff2 = dstoffs[1], dstdim2 = dstdims[1];                          \
    long dstoff3 = dstoffs[2];                                                \
    long srcoff1 = srcoffs[0], srcdim1 = srcdims[0];                          \
    long srcoff2 = srcoffs[1], srcdim2 = srcdims[1];                          \
    long srcoff3 = srcoffs[2];                                                \
    long len1 = lens[0];                                                      \
    long len2 = lens[1];                                                      \
    long len3 = lens[2];                                                      \
    for (long i3 = 0; i3 < len3; ++i3) {                                      \
        for (long i2 = 0; i2 < len2; ++i2) {                                  \
            DST* dst = ((DST*)dstptr) + OFF3(dst,i2,i3);                      \
            const SRC* src = ((SRC*)srcptr) + OFF3(src,i2,i3);                \
            for (long i1 = 0; i1 < len1; ++i1) {                              \
                dst[i1] = src[i1];                                            \
            }                                                                 \
        }                                                                     \
    }                                                                         \
}

#define ENCODE4(PFX, DST, SRC)                                                \
static void                                                                   \
copy_4d_##PFX(void* dstptr, const long dstdims[], const long dstoffs[],       \
              const void* srcptr, const long srcdims[], const long srcoffs[], \
              const long lens[])                                              \
{                                                                             \
    long dstoff1 = dstoffs[0], dstdim1 = dstdims[0];                          \
    long dstoff2 = dstoffs[1], dstdim2 = dstdims[1];                          \
    long dstoff3 = dstoffs[2], dstdim3 = dstdims[2];                          \
    long dstoff4 = dstoffs[3];                                                \
    long srcoff1 = srcoffs[0], srcdim1 = srcdims[0];                          \
    long srcoff2 = srcoffs[1], srcdim2 = srcdims[1];                          \
    long srcoff3 = srcoffs[2], srcdim3 = srcdims[2];                          \
    long srcoff4 = srcoffs[3];                                                \
    long len1 = lens[0];                                                      \
    long len2 = lens[1];                                                      \
    long len3 = lens[2];                                                      \
    long len4 = lens[3];                                                      \
    for (long i4 = 0; i4 < len4; ++i4) {                                      \
        for (long i3 = 0; i3 < len3; ++i3) {                                  \
            for (long i2 = 0; i2 < len2; ++i2) {                              \
                DST* dst = ((DST*)dstptr) + OFF4(dst,i2,i3,i4);               \
                const SRC* src = ((SRC*)srcptr) + OFF4(src,i2,i3,i4);         \
                for (long i1 = 0; i1 < len1; ++i1) {                          \
                    dst[i1] = src[i1];                                        \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }                                                                         \
}

#define ENCODE5(PFX, DST, SRC)                                                \
static void                                                                   \
copy_5d_##PFX(void* dstptr, const long dstdims[], const long dstoffs[],       \
              const void* srcptr, const long srcdims[], const long srcoffs[], \
              const long lens[])                                              \
{                                                                             \
    long dstoff1 = dstoffs[0], dstdim1 = dstdims[0];                          \
    long dstoff2 = dstoffs[1], dstdim2 = dstdims[1];                          \
    long dstoff3 = dstoffs[2], dstdim3 = dstdims[2];                          \
    long dstoff4 = dstoffs[3], dstdim4 = dstdims[3];                          \
    long dstoff5 = dstoffs[4];                                                \
    long srcoff1 = srcoffs[0], srcdim1 = srcdims[0];                          \
    long srcoff2 = srcoffs[1], srcdim2 = srcdims[1];                          \
    long srcoff3 = srcoffs[2], srcdim3 = srcdims[2];                          \
    long srcoff4 = srcoffs[3], srcdim4 = srcdims[3];                          \
    long srcoff5 = srcoffs[4];                                                \
    long len1 = lens[0];                                                      \
    long len2 = lens[1];                                                      \
    long len3 = lens[2];                                                      \
    long len4 = lens[3];                                                      \
    long len5 = lens[4];                                                      \
    for (long i5 = 0; i5 < len5; ++i5) {                                      \
        for (long i4 = 0; i4 < len4; ++i4) {                                  \
            for (long i3 = 0; i3 < len3; ++i3) {                              \
                for (long i2 = 0; i2 < len2; ++i2) {                          \
                    DST* dst = ((DST*)dstptr) + OFF5(dst,i2,i3,i4,i5);        \
                    const SRC* src = ((SRC*)srcptr) + OFF5(src,i2,i3,i4,i5);  \
                    for (long i1 = 0; i1 < len1; ++i1) {                      \
                        dst[i1] = src[i1];                                    \
                    }                                                         \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }                                                                         \
}

#define ENCODE_ALL_DIMS(FROM, TO)               \
    ENCODE1(FROM##_##TO, TYPE(TO), TYPE(FROM))  \
    ENCODE2(FROM##_##TO, TYPE(TO), TYPE(FROM))  \
    ENCODE3(FROM##_##TO, TYPE(TO), TYPE(FROM))  \
    ENCODE4(FROM##_##TO, TYPE(TO), TYPE(FROM))  \
    ENCODE5(FROM##_##TO, TYPE(TO), TYPE(FROM))

#define ENCODE_ALL_DIMS_ALL_SRCS(TO)            \
    ENCODE_ALL_DIMS(i8,  TO)                    \
    ENCODE_ALL_DIMS(u8,  TO)                    \
    ENCODE_ALL_DIMS(i16, TO)                    \
    ENCODE_ALL_DIMS(u16, TO)                    \
    ENCODE_ALL_DIMS(i32, TO)                    \
    ENCODE_ALL_DIMS(u32, TO)                    \
    ENCODE_ALL_DIMS(i64, TO)                    \
    ENCODE_ALL_DIMS(u64, TO)                    \
    ENCODE_ALL_DIMS(f32, TO)                    \
    ENCODE_ALL_DIMS(f64, TO)

ENCODE_ALL_DIMS_ALL_SRCS(i8)
ENCODE_ALL_DIMS_ALL_SRCS(u8)
ENCODE_ALL_DIMS_ALL_SRCS(i16)
ENCODE_ALL_DIMS_ALL_SRCS(u16)
ENCODE_ALL_DIMS_ALL_SRCS(i32)
ENCODE_ALL_DIMS_ALL_SRCS(u32)
ENCODE_ALL_DIMS_ALL_SRCS(i64)
ENCODE_ALL_DIMS_ALL_SRCS(u64)
ENCODE_ALL_DIMS_ALL_SRCS(f32)
ENCODE_ALL_DIMS_ALL_SRCS(f64)

#undef ENCODE
#undef ENCODE1
#undef ENCODE2
#undef ENCODE3
#undef ENCODE4
#undef ENCODE5

typedef void copy_proc(void* dstptr, const long dstdims[],
                       const long dstoffs[], const void* srcptr,
                       const long srcdims[], const long srcoffs[],
                       const long lens[]);

#define PROCS(d)    \
    PROCS1(d, i8),  \
    PROCS1(d, u8),  \
    PROCS1(d, i16), \
    PROCS1(d, u16), \
    PROCS1(d, i32), \
    PROCS1(d, u32), \
    PROCS1(d, i64), \
    PROCS1(d, u64), \
    PROCS1(d, f32), \
    PROCS1(d, f64)

#define PROCS1(d,t)         \
    copy_##d##d_##t##_i8,   \
    copy_##d##d_##t##_u8,   \
    copy_##d##d_##t##_i16,  \
    copy_##d##d_##t##_u16,  \
    copy_##d##d_##t##_i32,  \
    copy_##d##d_##t##_u32,  \
    copy_##d##d_##t##_i64,  \
    copy_##d##d_##t##_u64,  \
    copy_##d##d_##t##_f32,  \
    copy_##d##d_##t##_f64

static copy_proc* copy_proc_table[] = {
    PROCS(1), PROCS(2), PROCS(3), PROCS(4), PROCS(5)
};

#define NTYPES (TAO_FLOAT64 - TAO_INT8 + 1)
#define PROC_INDEX(rank,from,to) \
    ((((rank) - 1)*NTYPES + ((from) - TAO_INT8))*NTYPES + ((to) - TAO_INT8))

int
tao_copy(tao_error_t** errs,
         void* dstptr, tao_element_type_t dsttype,
         const long dstdims[], const long dstoffs[],
         const void* srcptr, tao_element_type_t srctype,
         const long srcdims[], const long srcoffs[],
         const long lens[], int ndims)
{
    if (ndims < 1 || ndims > TAO_MAX_NDIMS) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    if (dstptr == NULL || dstdims == NULL || dstoffs == NULL ||
        srcptr == NULL || srcdims == NULL || srcoffs == NULL ||
        lens == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (dsttype < TAO_INT8 || dsttype > TAO_FLOAT64 ||
        srctype < TAO_INT8 || srctype > TAO_FLOAT64) {
        tao_push_error(errs, __func__, TAO_BAD_TYPE);
        return -1;
    }
    for (int d = 0; d < ndims; ++d) {
        if (dstdims[d] < 1 || srcdims[d] < 1 || lens[d] < 1) {
            tao_push_error(errs, __func__, TAO_BAD_SIZE);
            return -1;
        }
        if (dstoffs[d] < 0 || dstoffs[d] + lens[d] > dstdims[d] ||
            srcoffs[d] < 0 || srcoffs[d] + lens[d] > srcdims[d]) {
            tao_push_error(errs, __func__, TAO_BAD_ROI);
            return -1;
        }
    }
    int k = PROC_INDEX(ndims, srctype, dsttype);
    (*copy_proc_table[k])(dstptr, dstdims, dstoffs,
                          srcptr, srcdims, srcoffs,
                          lens);
    return 0;
}

void
tao_copy_checked_args(void* dstptr, tao_element_type_t dsttype,
                      const long dstdims[], const long dstoffs[],
                      const void* srcptr, tao_element_type_t srctype,
                      const long srcdims[], const long srcoffs[],
                      const long lens[], int ndims)
{
    int k = PROC_INDEX(ndims, srctype, dsttype);
    (*copy_proc_table[k])(dstptr, dstdims, dstoffs,
                          srcptr, srcdims, srcoffs,
                          lens);
}
