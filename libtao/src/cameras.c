/*
 * cameras.c --
 *
 * Management of (shared) camera data.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

#include "common.h"

#include <math.h>
#include <errno.h>

#include "tao-private.h"

#define NEW(errs, type, number) \
    ((type*)tao_calloc(errs, number, sizeof(type)))

#define NEW_SHARED_OBJECT(errs, type, id, perms) \
    ((type*)tao_create_shared_object(errs, id, sizeof(type), perms))

#define LOCK_SHARED_CAMERA(errs, cam)             \
    tao_lock_mutex(errs, &(cam)->base.mutex)

#define TRY_LOCK_SHARED_CAMERA(errs, cam) \
    tao_try_lock_mutex(errs, &(cam)->base.mutex)

#define UNLOCK_SHARED_CAMERA(errs, cam) \
    tao_unlock_mutex(errs, &(cam)->base.mutex)

int
tao_finalize_camera(tao_error_t** errs, tao_camera_t* cam)
{
    int status = 0;
    if (cam != NULL) {
       if (cam->shared != NULL) {
            tao_shared_object_t* obj = (tao_shared_object_t*)cam->shared;
            cam->shared = NULL;
            if (tao_detach_shared_object(errs, obj) != 0) {
                status = -1;
            }
        }
        if (cam->frames != NULL) {
            for (int i = 0; i < cam->nframes; ++i) {
                tao_shared_array_t* arr = cam->frames[i];
                cam->frames[i] = NULL;
                if (arr != NULL && tao_detach_shared_array(errs, arr) != 0) {
                    status = -1;
                }
            }
            free(cam->frames);
        }
        if (cam->spare != NULL) {
            tao_shared_array_t* arr = cam->spare;
            cam->spare = NULL;
            if (tao_detach_shared_array(errs, arr) != 0) {
                status = -1;
            }
        }
        free(cam);
    }
    return status;
}

tao_camera_t*
tao_create_camera(tao_error_t** errs, int nframes, unsigned int perms)
{
    tao_camera_t* cam;
    tao_shared_camera_t* shared;

    if (nframes < 2) {
        tao_push_error(errs, __func__, TAO_BAD_ARGUMENT);
        return NULL;
    }
    cam = NEW(errs, tao_camera_t, 1);
    if (cam == NULL) {
        return NULL;
    }
    cam->perms = perms;
    cam->frames = NEW(errs, tao_shared_array_t*, nframes);
    if (cam->frames == NULL) {
        tao_finalize_camera(errs, cam);
        return NULL;
    }
    cam->nframes = nframes;
    cam->index = -1;
    shared = NEW_SHARED_OBJECT(errs, tao_shared_camera_t,
                               TAO_SHARED_CAMERA, perms);
    if (shared == NULL) {
        tao_finalize_camera(errs, cam);
        return NULL;
    }
    for (int i = 0; i < TAO_SHARED_CAMERA_SEMAPHORES; ++i) {
        if (sem_init(&shared->sem[i], TRUE, 0) != 0) {
            /* The only possible error here is that the system does not support
               process-shared semaphores.  This could be treated as a fatal
               error because this feature is a requirement in TAO.  We
               nevertheless be nice and gently report the error.  However, to
               avoid that tao_detach_shared_object calls sem_destroy, we first
               undo what has been done and pretend that the shared camera is a
               basic object. */
            tao_push_system_error(errs, "sem_init");
            while (--i >= 0) {
                (void)sem_destroy(&shared->sem[i]);
            }
            shared->base.type = TAO_SHARED_OBJECT;
            tao_detach_shared_object(errs, (tao_shared_object_t*)shared);
            tao_finalize_camera(errs, cam);
            return NULL;
        }
    }
    cam->shared = shared;
    shared->last_frame.ident = -1;
    shared->last_frame.counter = -1;
    shared->state = 0;
    shared->pixel_type = TAO_FLOAT32;
    shared->depth = 8;
    shared->fullwidth = 384;
    shared->fullheight = 288;
    shared->xoff = 0;
    shared->yoff = 0;
    shared->width = 1;
    shared->height = 1;
    shared->exposure = 0.001; /* 1 ms */
    shared->rate = 25.0; /* 25 Hz */
    shared->gain = 100.0;
    shared->bias = 500.0;
    shared->gamma = 1.0;
    return cam;
}

static tao_shared_array_t*
allocate_frame(tao_error_t** errs, tao_camera_t* cam)
{
    tao_shared_camera_t* shared = cam->shared;
    return (shared->weighted ?
            tao_create_3d_shared_array(errs,
                                       shared->pixel_type,
                                       shared->width,
                                       shared->height,
                                       2,
                                       cam->perms) :
            tao_create_2d_shared_array(errs,
                                       shared->pixel_type,
                                       shared->width,
                                       shared->height,
                                       cam->perms));
}

static int
check_frame(const tao_shared_array_t* arr, const tao_shared_camera_t* cam)
{
    return (arr->eltype == cam->pixel_type &&
            (cam->weighted ? (arr->ndims == 3 && arr->dims[2] == 2) :
             (arr->ndims == 2)) && arr->dims[0] == cam->width &&
            arr->dims[1] == cam->height);
}

/* WARNING: Caller must have locked the shared camera. */
tao_shared_array_t*
tao_fetch_next_frame(tao_error_t** errs, tao_camera_t* cam)
{
    /* Check arguments. */
    if (cam == NULL || cam->shared == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return NULL;
    }

    /* Local variables. */
    tao_shared_array_t** frame_list = cam->frames;
    tao_shared_camera_t* shared = cam->shared;
    int nframes = cam->nframes;
    int index = (cam->index + 1)%nframes; /* index of next frame */
    tao_shared_array_t* arr = frame_list[index];

    if (arr != NULL) {
        int drop = 0;
        if (arr->base.ident == shared->last_frame.ident) {
            /* We do not want to overwrite the previous last frame. */
            drop = 1;
        } else if (! check_frame(arr, shared)) {
            /* Array must have the correct element type and dimensions. */
            drop = 1;
        } else {
            /* Make sure array is not used by others. */
            if (tao_lock_shared_array(errs, arr) != 0) {
                return NULL;
            }
            if (arr->nreaders != 0 || arr->nwriters != 0) {
                drop = 1;
            } else {
                arr->nwriters = 1;
            }
            if (tao_unlock_shared_array(errs, arr) != 0) {
                return NULL;
            }
        }
        if (drop) {
            frame_list[index] = NULL;
            if (tao_detach_shared_array(errs, arr) != 0) {
                return NULL;
            }
            arr = NULL;
        }
    }

    if (arr == NULL) {
        if (cam->spare != NULL) {
            /* Attempt to use the pre-allocated array. */
            arr = cam->spare;
            cam->spare = NULL;
            if (check_frame(arr, shared)) {
                if (tao_detach_shared_array(errs, arr) != 0) {
                    return NULL;
                }
                arr = NULL;
            }
        }
        if (arr == NULL) {
            /* Create a new array. */
            arr = allocate_frame(errs, cam);
            if (arr == NULL) {
                return NULL;
            }
        }
        arr->nwriters = 1;
        frame_list[index] = arr;
    }
    cam->index = index;
    return arr;
}

int
tao_publish_next_frame(tao_error_t** errs, tao_camera_t* cam,
                       tao_shared_array_t* arr)
{
    /* Check arguments.  FIXME: We may also want to check that arr is
       cam->frames[cam->index]. */
    if (cam == NULL || arr == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    tao_shared_camera_t* shared = cam->shared;
    if (check_frame(arr, shared)) {
        tao_push_error(errs, __func__, TAO_BAD_ARGUMENT);
        return -1;
    }

    /* Increment image number and update image. */
    if (tao_lock_shared_array(errs, arr) != 0) {
        return -1;
    }
    int status = 0;
    if (arr->nreaders != 0 || arr->nwriters != 1) {
        tao_push_error(errs, __func__, TAO_ASSERTION_FAILED);
        status = -1;
        goto unlock;
    }
    arr->nwriters = 0;
    arr->counter = ++shared->last_frame.counter;
    shared->last_frame.ident = arr->base.ident;
 unlock:
    if (tao_unlock_shared_array(errs, arr) != 0) {
        status = -1;
    }
    if (status != 0) {
        return -1;
    }

    /* Signal that a new image is available. */
    for (int i = 0; i < TAO_SHARED_CAMERA_SEMAPHORES; ++i) {
        int val;
        if (sem_getvalue(&shared->sem[i], &val) != 0) {
            tao_push_system_error(errs, "sem_getvalue");
            return -1;
        }
        if (val == 0) {
            if (sem_post(&shared->sem[i]) != 0) {
                tao_push_system_error(errs, "sem_post");
                return -1;
            }
        }
    }

    /* If no array has been pre-allocated, allocate one. */
    if (cam->spare == NULL) {
        cam->spare = allocate_frame(errs, cam);
        if (cam->spare == NULL) {
            return -1;
        }
    }
    return 0;
}

tao_shared_array_t*
tao_attach_last_image(tao_error_t** errs, tao_shared_camera_t* cam)
{
    /* Check arguments. */
    if (cam == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return NULL;
    }

    return (cam->last_frame.ident < 0 ? NULL :
            tao_attach_shared_array(errs, cam->last_frame.ident));
}

int
tao_get_shared_camera_ident(const tao_shared_camera_t* cam)
{
    return (TAO_LIKELY(cam != NULL) ? cam->base.ident : -1);
}

#define GETTER(type, member, def)                                       \
    type                                                                \
    tao_get_shared_camera_##member(const tao_shared_camera_t* cam)      \
    {                                                                   \
        return (TAO_LIKELY(cam != NULL) ? cam->member : (def));         \
    }
GETTER(int,    state, -1)
GETTER(int,    pixel_type, -1)
GETTER(int,    depth, 0)
GETTER(long,   fullwidth, 0)
GETTER(long,   fullheight, 0)
GETTER(long,   xoff, 0)
GETTER(long,   yoff, 0)
GETTER(long,   width, 0)
GETTER(long,   height, 0)
GETTER(double, bias, 0.0)
GETTER(double, gain, 0.0)
GETTER(double, rate, 0.0)
GETTER(double, exposure, 0.0)
GETTER(double, gamma, 0.0)
#undef GETTER

uint64_t
tao_get_last_image_counter(const tao_shared_camera_t* cam)
{
    return (TAO_LIKELY(cam != NULL) ? cam->last_frame.counter : -1);
}

int
tao_get_last_image_ident(const tao_shared_camera_t* cam)
{
    return (TAO_LIKELY(cam != NULL) ? cam->last_frame.ident : -1);
}

tao_shared_camera_t*
tao_attach_shared_camera(tao_error_t** errs, int ident)
{
    tao_shared_object_t* obj = tao_attach_shared_object(errs, ident,
                                                        TAO_SHARED_CAMERA);
    return (tao_shared_camera_t*)obj;
}

int
tao_detach_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    /* No needs to check address. */
    return tao_detach_shared_object(errs, &(cam)->base);
}

int
tao_lock_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    if (TAO_UNLIKELY(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return LOCK_SHARED_CAMERA(errs, cam);
}

int
tao_try_lock_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    if (TAO_UNLIKELY(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return TRY_LOCK_SHARED_CAMERA(errs, cam);
}

int
tao_unlock_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    if (TAO_UNLIKELY(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return UNLOCK_SHARED_CAMERA(errs, cam);
}

#define CODE(FUNC, INP, OUT)                                            \
    void FUNC(OUT* d, OUT* w, const INP* r, int n,                      \
              const OUT* a, const OUT* b, const OUT* u, const OUT* v)   \
    {                                                                   \
        const OUT zero = 0;                                             \
        const OUT one = 1;                                              \
        if (w != NULL) {                                                \
            if (u != NULL && v != NULL) {                               \
                for (int i = 0; i < n; ++i) {                           \
                    OUT x = ((OUT)r[i] - b[i])*a[i];                    \
                    d[i] = x;                                           \
                    w[i] = u[i]/((x > zero ? x : zero) + v[i]);         \
                }                                                       \
            } else {                                                    \
                for (int i = 0; i < n; ++i) {                           \
                    d[i] = ((OUT)r[i] - b[i])*a[i];                     \
                    w[i] = one;                                         \
                }                                                       \
            }                                                           \
        } else {                                                        \
            for (int i = 0; i < n; ++i) {                               \
                d[i] = ((OUT)r[i] - b[i])*a[i];                         \
            }                                                           \
        }                                                               \
    }

CODE(tao_preprocess_image_u8_to_f32, uint8_t, float)
CODE(tao_preprocess_image_u8_to_f64, uint8_t, double)
CODE(tao_preprocess_image_u16_to_f32, uint16_t, float)
CODE(tao_preprocess_image_u16_to_f64, uint16_t, double)

#undef CODE

int
tao_wait_image(tao_error_t** errs, tao_shared_camera_t* cam, int idx)
{
    if (TAO_UNLIKELY(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (TAO_UNLIKELY(idx < 1 || idx > TAO_SHARED_CAMERA_SEMAPHORES)) {
        tao_push_error(errs, __func__, TAO_OUT_OF_RANGE);
        return -1;
    }
    if (sem_wait(&cam->sem[idx - 1]) != 0) {
        tao_push_system_error(errs, "sem_wait");
        return -1;
    }
    return 0;
}

int
tao_try_wait_image(tao_error_t** errs, tao_shared_camera_t* cam, int idx)
{
    if (TAO_UNLIKELY(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (TAO_UNLIKELY(idx < 1 || idx > TAO_SHARED_CAMERA_SEMAPHORES)) {
        tao_push_error(errs, __func__, TAO_OUT_OF_RANGE);
        return -1;
    }
    if (sem_trywait(&cam->sem[idx - 1]) != 0) {
        int code = errno;
        if (code == EAGAIN) {
            return 0;
        }
        tao_push_error(errs, "sem_trywait", code);
        return -1;
    }
    return 1;
}

int
tao_timed_wait_image(tao_error_t** errs, tao_shared_camera_t* cam, int idx,
                     double secs)
{
    if (TAO_UNLIKELY(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (TAO_UNLIKELY(idx < 1 || idx > TAO_SHARED_CAMERA_SEMAPHORES)) {
        tao_push_error(errs, __func__, TAO_OUT_OF_RANGE);
        return -1;
    }
    if (isnan(secs) || secs < 0) {
        tao_push_error(errs, __func__, TAO_BAD_ARGUMENT);
        return -1;
    }
    if (secs < 1e-9) {
        /* For a very short timeout, we just call `sem_trywait`. */
        if (sem_trywait(&cam->sem[idx - 1]) != 0) {
            int code = errno;
            if (code == EAGAIN) {
                /* Pretend this was due to a timed-out. */
                code = ETIMEDOUT;
                return 0;
            }
            tao_push_error(errs, "sem_trywait", code);
            return -1;
        }
    } else if (secs > TAO_YEAR) {
        /* For a very long timeout (the above limit is about one year), we just
           call `sem_wait`. */
        if (sem_wait(&cam->sem[idx - 1]) != 0) {
            tao_push_system_error(errs, "sem_wait");
            return -1;
        }
    } else {
        tao_time_t t;
        struct timespec ts;
        if (tao_get_absolute_timeout(errs, &t, secs) != 0) {
            return -1;
        }
        ts.tv_sec  = t.s;
        ts.tv_nsec = t.ns;
        if (sem_timedwait(&cam->sem[idx - 1], &ts) != 0) {
            int code = errno;
            if (code == ETIMEDOUT) {
                return 0;
            }
            tao_push_error(errs, "sem_timedwait", code);
            return -1;
        }
    }
    return 1;
}
