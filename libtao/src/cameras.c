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

#include "tao-private.h"

#define NEW(errs, type, number) \
    ((type*)tao_calloc(errs, number, sizeof(type)))

#define NEW_SHARED_OBJECT(errs, type, id, perms) \
    ((type*)tao_create_shared_object(errs, id, sizeof(type), perms))

#define ATTACH_SHARED_CAMERA(errs, ident) \
    ((tao_shared_camera_t*)tao_attach_shared_object(errs, ident, \
                                                    TAO_SHARED_CAMERA))

#define DETACH_SHARED_CAMERA(errs, cam) \
    tao_detach_shared_object(errs, (tao_shared_object_t*)(cam))

#define LOCK_SHARED_CAMERA(errs, cam)             \
    tao_lock_mutex(errs, &(cam)->base.mutex)

#define TRY_LOCK_SHARED_CAMERA(errs, cam) \
    tao_try_lock_mutex(errs, &(cam)->base.mutex)

#define UNLOCK_SHARED_CAMERA(errs, cam) \
    tao_unlock_mutex(errs, &(cam)->base.mutex)

int
tao_finalize_camera(tao_error_t** errs, tao_camera_t* cam)
{
    if (cam != NULL) {
        if (cam->shared != NULL) {
            tao_shared_object_t* obj = (tao_shared_object_t*)cam->shared;
            cam->shared =  NULL;
            tao_detach_shared_object(errs, obj);
        }
        if (cam->frames != NULL) {
            for (int i = 0; i < cam->nframes; ++i) {
                tao_shared_array_t* arr = cam->frames[i];
                cam->frames[i] = NULL;
                tao_detach_shared_array(errs, arr);
            }
            free(cam->frames);
        }
        free(cam);
    }
}

tao_camera_t*
tao_create_camera(tao_error_t** errs, int nframes, unsigned int perms)
{
    tao_camera_t* cam;
    tao_shared_camera_t* shared;
    tao_shared_object_t* obj;

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
    cam->shared = shared;
    shared->last_frame.ident = -1;
    shared->last_frame.counter = -1;
    shared->state = 0;
    shared->pixel_type = TAO_FLOAT32;
    shared->xoff = 0;
    shared->yoff = 0;
    shared->width = 1;
    shared->height = 1;
    return cam;
}

tao_shared_array_t*
tao_fetch_next_frame(tao_error_t** errs, tao_camera_t* cam)
{
    tao_shared_array_t* arr;
    tao_shared_array_t** frame_list = cam->frames;
    tao_shared_camera_t* shared = cam->shared;
    int nframes = cam->nframes;
    int index1, index2, index3;

    /* Find, preferentially, a recyclable array, otherwise the index of an
       empty slot in the list of shared arrays, otherwise the oldest array.
       With this strategy, the number of allocated images is maintained to a
       mimimum. */
    index1 = -1; /* first choice, a frame which can be recycled */
    index2 = -1; /* second choice, an empty slot */
    index3 = -1; /* third choice, the oldest frame */
    if (LOCK_SHARED_CAMERA(errs, shared) != 0) {
        goto error;
    }
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
                /* FIXME: this is probably a fatal error */
                goto unlock_and_error;
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
            /* FIXME: this is probably a fatal error */
            goto unlock_and_error;
        }
        index2 = index3;
    }
    if (UNLOCK_SHARED_CAMERA(errs, shared) != 0) {
        /* FIXME: this is probably a fatal error */
        goto error;
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

 unlock_and_error:
    UNLOCK_SHARED_CAMERA(errs, shared);
 error:
    return NULL;
}

#if 0
/* Perform pre-processing. */
for (int i = 0; i < NPIXELS; ++i) {
    img->data[i] = raw->data[i];
 }
#endif

int
tao_publish_next_frame(tao_error_t** errs, tao_camera_t* cam,
                       tao_shared_array_t* arr)
{
    tao_shared_camera_t* shared = cam->shared;
    if (LOCK_SHARED_CAMERA(errs, shared) != 0) {
        return -1;
    }

    /* FIXME: check information (pixel size, etc.) */
    /* Increment image number and update image. */
    /* FIXME: timestamp */
    /* FIXME: lock array or assumed it has been locked for the processing? */
    arr->counter = ++shared->last_frame.counter;
    arr->nwriters = 0;
    shared->last_frame.ident = arr->base.ident;

#if 0
    /* FIXME: Signal that a new image is available to all waiting processes. */
    signalNewImage(shared);
#endif

    if (UNLOCK_SHARED_CAMERA(errs, shared) != 0) {
        return -1;
    }
    return 0;
}

tao_shared_array_t*
tao_attach_last_image(tao_error_t** errs, tao_shared_camera_t* cam)
{
    tao_shared_array_t* arr;

    if (TAO_ANY_ERRORS(errs)) {
        return NULL;
    }
#if 1
    if (cam->last_frame.ident < 0) {
        return NULL;
    }
    return tao_attach_shared_array(errs, cam->last_frame.ident);
#else
    if (LOCK_SHARED_CAMERA(errs, cam) != 0) {
        return NULL;
    }
    if (cam->last_frame.ident >= 0) {
        arr = tao_attach_shared_array(errs, cam->last_frame.ident);
    } else {
        arr = NULL;
    }
    if (UNLOCK_SHARED_CAMERA(errs, cam) != 0) {
        tao_detach_shared_array(errs, arr);
        return NULL;
    }
    return arr;
#endif
}

uint64_t
tao_get_last_image_counter(tao_shared_camera_t* cam)
{
    return cam->last_frame.counter;
}

int
tao_get_last_image_ident(tao_shared_camera_t* cam)
{
    return cam->last_frame.ident;
}

tao_shared_camera_t*
tao_attach_shared_camera(tao_error_t** errs, int ident)
{
    return ATTACH_SHARED_CAMERA(errs, ident);
}

int
tao_detach_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    return DETACH_SHARED_CAMERA(errs, cam);
}

int
tao_lock_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    return LOCK_SHARED_CAMERA(errs, cam);
}

int
tao_try_lock_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    return TRY_LOCK_SHARED_CAMERA(errs, cam);
}

int
tao_unlock_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    return UNLOCK_SHARED_CAMERA(errs, cam);
}
