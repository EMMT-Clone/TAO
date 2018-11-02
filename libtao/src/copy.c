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

#define OFF2(S,i2)               (i2)*S##dim1
#define OFF3(S,i2,i3)            OFF2(S, (i3)*S##dim2 + (i2))
#define OFF4(S,i2,i3,i4)         OFF3(S, i2, (i4)*S##dim3 + (i3))
#define OFF5(S,i2,i3,i4,i5)      OFF4(S, i2, i3, (i5)*S##dim4 + (i4))

typedef void fastcopy_proc(void* dst, long dstoff,
                           const void* src, long srcoff, long len);

typedef void copy_proc(void* dstdata, const long dstdims[],
                       const void* srcdata, const long srcdims[],
                       const long lens[], fastcopy_proc *fastcopy);

#define FASTCOPY1(PFX, DST, SRC)                                \
    static void                                                 \
    fastcopy_##PFX(void* dstdata, long dstoff,                  \
                   const void* srcdata, long srcoff,            \
                   long len)                                    \
    {                                                           \
        DST* dst = ((DST*)dstdata) + dstoff;                    \
        const SRC* src = ((const SRC*)srcdata) + srcoff;        \
        for (long i = 0; i < len; ++i) {                        \
            dst[i] = src[i];                                    \
        }                                                       \
    }

#define FASTCOPY(from, to) FASTCOPY1(from##_##to, TYPE(to), TYPE(from))

#define FASTCOPY_PROCS(from)                    \
    FASTCOPY(from, i8)                          \
    FASTCOPY(from, u8)                          \
    FASTCOPY(from, i16)                         \
    FASTCOPY(from, u16)                         \
    FASTCOPY(from, i32)                         \
    FASTCOPY(from, u32)                         \
    FASTCOPY(from, i64)                         \
    FASTCOPY(from, u64)                         \
    FASTCOPY(from, f32)                         \
    FASTCOPY(from, f64)

FASTCOPY_PROCS(i8)
FASTCOPY_PROCS(u8)
FASTCOPY_PROCS(i16)
FASTCOPY_PROCS(u16)
FASTCOPY_PROCS(i32)
FASTCOPY_PROCS(u32)
FASTCOPY_PROCS(i64)
FASTCOPY_PROCS(u64)
FASTCOPY_PROCS(f32)
FASTCOPY_PROCS(f64)

#undef FASTCOPY_PROCS
#undef FASTCOPY1
#undef FASTCOPY

#if TAO_MAX_NDIMS >= 2
static void
copy_2d(void* dstdata, const long dstdims[],
        const void* srcdata, const long srcdims[],
        const long lens[], fastcopy_proc *fastcopy)
{
    long len1 = lens[0], dstdim1 = dstdims[0], srcdim1 = srcdims[0];
    long len2 = lens[1];
    for (long i2 = 0; i2 < len2; ++i2) {
        fastcopy(dstdata, OFF2(dst,i2),
                 srcdata, OFF2(src,i2), len1);
    }
}
#endif

#if TAO_MAX_NDIMS >= 3
static void
copy_3d(void* dstdata, const long dstdims[],
        const void* srcdata, const long srcdims[],
        const long lens[], fastcopy_proc *fastcopy)
{
    long len1 = lens[0], dstdim1 = dstdims[0], srcdim1 = srcdims[0];
    long len2 = lens[1], dstdim2 = dstdims[1], srcdim2 = srcdims[1];
    long len3 = lens[2];
    for (long i3 = 0; i3 < len3; ++i3) {
        for (long i2 = 0; i2 < len2; ++i2) {
            fastcopy(dstdata, OFF3(dst,i2,i3),
                     srcdata, OFF3(src,i2,i3), len1);
        }
    }
}
#endif

#if TAO_MAX_NDIMS >= 4
static void
copy_4d(void* dstdata, const long dstdims[],
        const void* srcdata, const long srcdims[],
        const long lens[], fastcopy_proc *fastcopy)
{
    long len1 = lens[0], dstdim1 = dstdims[0], srcdim1 = srcdims[0];
    long len2 = lens[1], dstdim2 = dstdims[1], srcdim2 = srcdims[1];
    long len3 = lens[2], dstdim3 = dstdims[2], srcdim3 = srcdims[2];
    long len4 = lens[3];
    for (long i4 = 0; i4 < len4; ++i4) {
        for (long i3 = 0; i3 < len3; ++i3) {
            for (long i2 = 0; i2 < len2; ++i2) {
                fastcopy(dstdata, OFF4(dst,i2,i3,i4),
                         srcdata, OFF4(src,i2,i3,i4), len1);
            }
        }
    }
}
#endif

#if TAO_MAX_NDIMS >= 5
static void
copy_5d(void* dstdata, const long dstdims[],
        const void* srcdata, const long srcdims[],
        const long lens[], fastcopy_proc *fastcopy)
{
    long len1 = lens[0], dstdim1 = dstdims[0], srcdim1 = srcdims[0];
    long len2 = lens[1], dstdim2 = dstdims[1], srcdim2 = srcdims[1];
    long len3 = lens[2], dstdim3 = dstdims[2], srcdim3 = srcdims[2];
    long len4 = lens[3], dstdim4 = dstdims[3], srcdim4 = srcdims[3];
    long len5 = lens[4];
    for (long i5 = 0; i5 < len5; ++i5) {
        for (long i4 = 0; i4 < len4; ++i4) {
            for (long i3 = 0; i3 < len3; ++i3) {
                for (long i2 = 0; i2 < len2; ++i2) {
                    fastcopy(dstdata, OFF5(dst,i2,i3,i4,i5),
                             srcdata, OFF5(src,i2,i3,i4,i5), len1);
                }
            }
        }
    }
}
#endif

#if TAO_MAX_NDIMS > 5
#  error code for more than 5 dimensions is not yet implemented
#endif

#define FASTCOPY_PROCS(t) \
    fastcopy_##t##_i8,    \
    fastcopy_##t##_u8,    \
    fastcopy_##t##_i16,   \
    fastcopy_##t##_u16,   \
    fastcopy_##t##_i32,   \
    fastcopy_##t##_u32,   \
    fastcopy_##t##_i64,   \
    fastcopy_##t##_u64,   \
    fastcopy_##t##_f32,   \
    fastcopy_##t##_f64

static fastcopy_proc* fastcopy_proc_table[] = {
    FASTCOPY_PROCS(i8),  \
    FASTCOPY_PROCS(u8),  \
    FASTCOPY_PROCS(i16), \
    FASTCOPY_PROCS(u16), \
    FASTCOPY_PROCS(i32), \
    FASTCOPY_PROCS(u32), \
    FASTCOPY_PROCS(i64), \
    FASTCOPY_PROCS(u64), \
    FASTCOPY_PROCS(f32), \
    FASTCOPY_PROCS(f64)
};

#undef FASTCOPY_PROCS

static copy_proc* copy_proc_table[] = {
#if TAO_MAX_NDIMS >= 2
    copy_2d,
#endif
#if TAO_MAX_NDIMS >= 3
    copy_3d,
#endif
#if TAO_MAX_NDIMS >= 4
    copy_4d,
#endif
#if TAO_MAX_NDIMS >= 5
    copy_5d,
#endif
    NULL
};

/* Number of different types. */
#define NTYPES (TAO_FLOAT64 - TAO_INT8 + 1)

int
tao_copy(tao_error_t** errs,
         void* dstdata, tao_element_type_t dsttype,
         const long dstdims[], const long dstoffs[],
         const void* srcdata, tao_element_type_t srctype,
         const long srcdims[], const long srcoffs[],
         const long lens[], int ndims)
{
    if (dstdata == NULL || dstdims == NULL ||
        srcdata == NULL || srcdims == NULL ||
        lens == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (ndims < 0 || ndims > TAO_MAX_NDIMS) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
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
    }
    if (dstoffs != NULL) {
        for (int d = 0; d < ndims; ++d) {
            if (dstoffs[d] < 0 || dstoffs[d] + lens[d] > dstdims[d]) {
                tao_push_error(errs, __func__, TAO_BAD_ROI);
                return -1;
            }
        }
    }
    if (srcoffs != NULL) {
        for (int d = 0; d < ndims; ++d) {
            if (srcoffs[d] < 0 || srcoffs[d] + lens[d] > srcdims[d]) {
                tao_push_error(errs, __func__, TAO_BAD_ROI);
                return -1;
            }
        }
    }
    tao_copy_checked_args(dstdata, dsttype, dstdims, dstoffs,
                          srcdata, srctype, srcdims, srcoffs,
                          lens, ndims);
    return 0;
}

void
tao_copy_checked_args(void* dstdata, tao_element_type_t dsttype,
                      const long dstdims[], const long dstoffs[],
                      const void* srcdata, tao_element_type_t srctype,
                      const long srcdims[], const long srcoffs[],
                      const long lens[], int ndims)
{
    /* Compute uni-dimensional offsets from the muti-dimensional offsets. */
    long dstoff = 0;
    long srcoff = 0;
    if (ndims > 0) {
        if (dstoffs != NULL) {
            int d = ndims - 1;
            dstoff = dstoffs[d];
            while (--d >= 0) {
                dstoff = dstoffs[d] + dstdims[d]*dstoff;
            }
        }
        if (srcoffs != NULL) {
            int d = ndims - 1;
            srcoff = srcoffs[d];
            while (--d >= 0) {
                srcoff = srcoffs[d] + srcdims[d]*srcoff;
            }
        }
    }

    /*
     * Divide the work to be done in 2 parts, a fast copy (with possible
     * conversion) along the inner dimension(s) and outer iterations over
     * the other indices.
     */
    int fastcopy;
    if (ndims <= 1) {
        fastcopy = TRUE;
    } else {
        fastcopy = (dstoff == 0 && srcoff == 0);
        if (fastcopy) {
            for (int d = 0; d < ndims; ++d) {
                if (dstdims[d] != lens[d] || srcdims[d] != lens[d]) {
                    fastcopy = FALSE;
                    break;
                }
            }
        }
    }
    int k1 = (srctype - TAO_INT8)*NTYPES + (dsttype - TAO_INT8);
    if (fastcopy) {
        long len = 1;
        for (int d = 0; d < ndims; ++d) {
            len *= lens[d];
        }
        (*fastcopy_proc_table[k1])(dstdata, dstoff,
                                   srcdata, srcoff, len);
    } else {
        long dstsiz = tao_get_element_size(dsttype);
        long srcsiz = tao_get_element_size(srctype);
        dstdata = (      void*)((      uint8_t*)dstdata + dstoff*dstsiz);
        srcdata = (const void*)((const uint8_t*)srcdata + srcoff*srcsiz);
        (*copy_proc_table[ndims - 2])(dstdata, dstdims,
                                      srcdata, srcdims,
                                      lens, fastcopy_proc_table[k1]);
    }
}

#define DATA(arr) (void*)((uint8_t*)(arr) + (arr)->offset)

int
tao_copy_to_array(tao_error_t** errs,
                  tao_array_t* dst, const long dstoffs[],
                  const void* srcdata, tao_element_type_t srctype,
                  const long srcdims[], const long srcoffs[],
                  const long lens[], int ndims)
{
    if (dst == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (dst->ndims != ndims) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    return tao_copy(errs, dst->data, dst->eltype, dst->dims, dstoffs,
                    srcdata, srctype, srcdims, srcoffs,
                    lens, ndims);
}

int
tao_copy_to_shared_array(tao_error_t** errs,
                         tao_shared_array_t* dst, const long dstoffs[],
                         const void* srcdata, tao_element_type_t srctype,
                         const long srcdims[], const long srcoffs[],
                         const long lens[], int ndims)
{
    if (dst == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (dst->ndims != ndims) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    return tao_copy(errs, DATA(dst), dst->eltype, dst->dims, dstoffs,
                    srcdata, srctype, srcdims, srcoffs,
                    lens, ndims);
}

int
tao_copy_from_array(tao_error_t** errs,
                    void* dstdata, tao_element_type_t dsttype,
                    const long dstdims[], const long dstoffs[],
                    tao_array_t* src, const long srcoffs[],
                    const long lens[], int ndims)
{
    if (src == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (src->ndims != ndims) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    return tao_copy(errs,
                    dstdata, dsttype, dstdims, dstoffs,
                    src->data, src->eltype, src->dims, srcoffs,
                    lens, ndims);
}

int
tao_copy_from_shared_array(tao_error_t** errs,
                           void* dstdata, tao_element_type_t dsttype,
                           const long dstdims[], const long dstoffs[],
                           tao_shared_array_t* src, const long srcoffs[],
                           const long lens[], int ndims)
{
    if (src == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (src->ndims != ndims) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    return tao_copy(errs,
                    dstdata, dsttype, dstdims, dstoffs,
                    DATA(src), src->eltype, src->dims, srcoffs,
                    lens, ndims);
}

int
tao_copy_array_to_array(tao_error_t** errs,
                        tao_array_t* dst, const long dstoffs[],
                        tao_array_t* src, const long srcoffs[],
                        const long lens[], int ndims)
{
    if (dst == NULL || src == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (dst->ndims != ndims || src->ndims != ndims) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    return tao_copy(errs,
                    dst->data, dst->eltype, dst->dims, dstoffs,
                    src->data, src->eltype, src->dims, srcoffs,
                    lens, ndims);
}

int
tao_copy_array_to_shared_array(tao_error_t** errs,
                               tao_shared_array_t* dst, const long dstoffs[],
                               tao_array_t* src, const long srcoffs[],
                               const long lens[], int ndims)
{
    if (dst == NULL || src == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (dst->ndims != ndims || src->ndims != ndims) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    return tao_copy(errs,
                    DATA(dst), dst->eltype, dst->dims, dstoffs,
                    src->data, src->eltype, src->dims, srcoffs,
                    lens, ndims);
}

int
tao_copy_shared_array_to_array(tao_error_t** errs,
                               tao_array_t* dst, const long dstoffs[],
                               tao_shared_array_t* src, const long srcoffs[],
                               const long lens[], int ndims)
{
    if (dst == NULL || src == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (dst->ndims != ndims || src->ndims != ndims) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    return tao_copy(errs,
                    dst->data, dst->eltype, dst->dims, dstoffs,
                    DATA(src), src->eltype, src->dims, srcoffs,
                    lens, ndims);
}

int
tao_copy_shared_array_to_shared_array(tao_error_t** errs,
                                      tao_shared_array_t* dst,
                                      const long dstoffs[],
                                      tao_shared_array_t* src,
                                      const long srcoffs[],
                                      const long lens[], int ndims)
{
    if (dst == NULL || src == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (dst->ndims != ndims || src->ndims != ndims) {
        tao_push_error(errs, __func__, TAO_BAD_RANK);
        return -1;
    }
    return tao_copy(errs,
                    DATA(dst), dst->eltype, dst->dims, dstoffs,
                    DATA(src), src->eltype, src->dims, srcoffs,
                    lens, ndims);
}
