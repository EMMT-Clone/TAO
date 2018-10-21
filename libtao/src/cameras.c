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
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <math.h>
#include <errno.h>

#include "config.h"
#include "macros.h"
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

tao_shared_array_t*
tao_fetch_next_frame(tao_error_t** errs, tao_camera_t* cam)
{
    /* Check arguments. */
    if (cam == NULL || cam->shared == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return NULL;
    }

    /* Local variables. */
    tao_shared_array_t* arr;
    tao_shared_array_t** frame_list = cam->frames;
    tao_shared_camera_t* shared = cam->shared;
    int nframes = cam->nframes;

    /* Find, preferentially, a recyclable array, otherwise the index of an
       empty slot in the list of shared arrays, otherwise the oldest array.
       With this strategy, the number of allocated images is maintained to a
       mimimum. */
    int index1 = -1; /* first choice, a frame which can be recycled */
    int index2 = -1; /* second choice, an empty slot */
    int index3 = -1; /* third choice, the oldest frame */
    for (int i = 0; i < nframes; ++i) {
        arr = frame_list[i];
        if (arr == NULL) {
            /* An empty slot is only our second choice. */
            if (index2 == -1) {
                index2 = i;
            }
            continue;
        }
        if (arr->base.ident == shared->last_frame.ident) {
            /* We do not want to overwrite the previous last frame. */
            continue;
        }
        if (arr->eltype != shared->pixel_type || arr->ndims != 2 ||
            arr->size[0] != shared->width || arr->size[1] != shared->height) {
            /* This frame does not have the correct format, discard it. */
            frame_list[i] = NULL;
            if (tao_detach_shared_array(errs, arr) != 0) {
                return NULL;
            }
            if (index2 == -1) {
                index2 = i;
            }
            continue;
        }
        if (arr->nreaders < 1 && arr->nwriters < 1 &&
            (index1 == -1 || arr->counter < frame_list[index1]->counter)) {
            /* This frame has currently no readers, nor writers and it is the
               first frame or the oldest frame satisfying all criteria. */
            index1 = i;
            continue;
        }
        if (index3 == -1 || arr->counter < frame_list[index3]->counter) {
            /* This is the oldest frame. */
            index3 = i;
        }
    }
    if (index1 >= 0) {
        /* Mark the chosen frame as being written. */
        /* FIXME: lock the frame? */
        frame_list[index1]->nwriters = 1;
    } else if (index2 < 0) {
        /* Detach the oldest frame. */
        tao_shared_array_t* tmp = frame_list[index3];
        if (tao_detach_shared_array(errs, tmp) != 0) {
            return NULL;
        }
        index2 = index3;
    }

    /* If no image can be recycled, allocate a new one. */
    if (index1 < 0) {
        /* New image has ident ≥ 1, number = 0 and nrefs = 1 */
        frame_list[index2] = tao_create_2d_shared_array(errs,
                                                        shared->pixel_type,
                                                        shared->width,
                                                        shared->height,
                                                        cam->perms);
        index1 = index2;
    }
    return frame_list[index1];
}

int
tao_publish_next_frame(tao_error_t** errs, tao_camera_t* cam,
                       tao_shared_array_t* arr)
{
    /* Check arguments. */
    if (cam == NULL || arr == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }

    /* Increment image number and update image. */
    /* FIXME: check information (pixel size, etc.) */
    /* FIXME: lock array or assume it has been locked for the processing? */
    tao_shared_camera_t* shared = cam->shared;
    arr->counter = ++shared->last_frame.counter;
    arr->nwriters = 0;
    shared->last_frame.ident = arr->base.ident;

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
    return (likely(cam != NULL) ? cam->base.ident : -1);
}

#define GETTER(type, member, def)                                       \
    type                                                                \
    tao_get_shared_camera_##member(const tao_shared_camera_t* cam)      \
    {                                                                   \
        return (likely(cam != NULL) ? cam->member : (def));             \
    }
GETTER(int, state, -1)
GETTER(int, pixel_type, -1)
GETTER(int, depth, 0)
GETTER(int, fullwidth, 0)
GETTER(int, fullheight, 0)
GETTER(int, xoff, 0)
GETTER(int, yoff, 0)
GETTER(int, width, 0)
GETTER(int, height, 0)
GETTER(double, bias, 0.0)
GETTER(double, gain, 0.0)
GETTER(double, rate, 0.0)
GETTER(double, exposure, 0.0)
GETTER(double, gamma, 0.0)
#undef GETTER

uint64_t
tao_get_last_image_counter(const tao_shared_camera_t* cam)
{
    return (likely(cam != NULL) ? cam->last_frame.counter : -1);
}

int
tao_get_last_image_ident(const tao_shared_camera_t* cam)
{
    return (likely(cam != NULL) ? cam->last_frame.ident : -1);
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
    if (unlikely(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return LOCK_SHARED_CAMERA(errs, cam);
}

int
tao_try_lock_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    if (unlikely(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return TRY_LOCK_SHARED_CAMERA(errs, cam);
}

int
tao_unlock_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    if (unlikely(cam == NULL)) {
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
    if (unlikely(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (unlikely(idx < 1 || idx > TAO_SHARED_CAMERA_SEMAPHORES)) {
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
    if (unlikely(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (unlikely(idx < 1 || idx > TAO_SHARED_CAMERA_SEMAPHORES)) {
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
    if (unlikely(cam == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    if (unlikely(idx < 1 || idx > TAO_SHARED_CAMERA_SEMAPHORES)) {
        tao_push_error(errs, __func__, TAO_OUT_OF_RANGE);
        return -1;
    }
    if (! isnan(secs) || secs < 0) {
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
    } else if (secs > 31.7e6) {
        /* For a very long timeout (the above limit is about one year), we just
           call `sem_wait`. */
        if (sem_wait(&cam->sem[idx - 1]) != 0) {
            tao_push_system_error(errs, "sem_wait");
            return -1;
        }
    } else {
        struct timespec ts;
        long s = floor(secs);
        long ns = lround((secs - s)*1e9);
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
            tao_push_system_error(errs, "clock_gettime");
            return -1;
        }
        ts.tv_sec += s;
        ts.tv_nsec += ns;
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
