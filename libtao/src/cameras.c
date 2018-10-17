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

/**
 * This structure describes the shared data storing the global resources of a
 * frame grabber.
 *
 */
typedef struct tao_shared_camera {
    tao_shared_object_t base;     /**< Shared object backing storage of the
                                       shared frame grabber */
    struct {
        int32_t ident;   /**< Identifier of the shared array backing the
                              storage of the last image, -1 means unused or
                              invalid. */
        int64_t counter; /**< Counter value of the last image.  It is a unique,
                              monotically increasing number, starting at 1 (0
                              means unused or invalid). */
    } last_frame;
    unsigned int perms;
    int state; /* 0 = device not yet open,
                * 1 = device open, no acquisition running,
                * 2 = acquisition running
                */
    int pixel_type;
    int xoff;
    int yoff;
    int width;
    int height;

} tao_shared_camera_t;

typedef struct tao_camera tao_camera_t;
typedef struct tao_camera {
    tao_shared_camera_t* shared; /**< Attached shared camera data. */
    int nframes;                 /**< Maximum number of memorized frames. */
    tao_shared_array_t** frames; /**< List of shared arrays. */
} tao_camera_t;

/* FIXME: The frame list must be stored somewhere, it can be in the shared
   camera data but this is dangerous (and the addresses in this array are only
   valid for the server who created the resources).  A possible fix is to have
   two structures: one for the server (with its own NFRAMES), the other to be
   shared.  This is what is drafted above but we need fucntion to manage this
   structure. */
#define NFRAMES 12
static tao_shared_array_t* frame_list[NFRAMES];

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

tao_shared_camera_t*
tao_create_shared_camera(tao_error_t** errs, unsigned int perms)
{
    size_t nbytes = sizeof(tao_shared_camera_t);
    tao_shared_object_t* obj;
    obj = tao_create_shared_object(errs, TAO_SHARED_CAMERA, nbytes, perms);
    if (obj == NULL) {
        return NULL;
    }
    tao_shared_camera_t* cam = (tao_shared_camera_t*)obj;
    cam->last_frame.ident = -1;
    cam->last_frame.counter = -1;
    cam->state = 0;
    cam->perms = perms;
    cam->pixel_type = TAO_FLOAT32;
    cam->xoff = 0;
    cam->yoff = 0;
    cam->width = 1;
    cam->height = 1;
    return cam;
}

tao_shared_camera_t*
tao_attach_shared_camera(tao_error_t** errs, int ident)
{
    return ATTACH_SHARED_CAMERA(errs, ident);
}

int
tao_detach_shared_camera(tao_error_t** errs, tao_shared_camera_t* cam)
{
    /* FIXME: cleanup if there are no references? */
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


/**
 * @addtogroup SharedCameras
 *
 * The management of every new frame is done in 3 steps.
 *
 * - First, a shared array is located or created to store the resulting data,
 *   if possible an old frame data is recycled.
 *
 * - Second, the raw frame data is processed and stored in the selected shared
 *   array.
 *
 * - Third, the shared array is updated (its counter is incremented) and is
 *   marked as being readable.
 *
 * The first step is done by calling tao_fetch_next_frame() and the third
 * steps is done by calling tao_release_next_frame().  These two steps are
 * performed while the shared camera data are locked.
 *
 * @{
 */

/**
 * Get shared array to store next camera image.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Address of the shared camera data.
 *
 * @return The address of a shared array to store the processed data frame;
 * `NULL` in case of errors.
 */
tao_shared_array_t*
tao_fetch_next_frame(tao_error_t** errs, tao_camera_t*cam);

/**
 * Get shared array to store next camera image.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Address of the camera data.
 * @param arr    Address of the shared frame data.
 *
 * @return `0` on success; `1` in case of errors.
 */
int
tao_release_next_frame(tao_error_t** errs, tao_camera_t* cam,
                       tao_shared_array_t* arr);

/** @} */

tao_shared_array_t*
tao_fetch_next_frame(tao_error_t** errs, tao_camera_t* cam)
{
    tao_shared_array_t* arr;
    tao_shared_array_t** frame_list = cam->frames;
    tao_shared_camera_t* shmcam = cam->shared;
    int nframes = cam->nframes;
    int index1, index2, index3;

    /* Find, preferentially, a recyclable array, otherwise the index of an
       empty slot in the list of shared arrays, otherwise the oldest array.
       With this strategy, the number of allocated images is maintained to a
       mimimum. */
    index1 = -1; /* first choice, a frame which can be recycled */
    index2 = -1; /* second choice, an empty slot */
    index3 = -1; /* third choice, the oldest frame */
    if (LOCK_SHARED_CAMERA(errs, shmcam) != 0) {
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
        if (arr->base.ident == shmcam->last_frame.ident) {
            /* We do not want to overwrite the previous last frame. */
            continue;
        }
        if (arr->eltype != shmcam->pixel_type || arr->ndims != 2 ||
            arr->size[0] != shmcam->width || arr->size[1] != shmcam->height) {
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
    if (UNLOCK_SHARED_CAMERA(errs, shmcam) != 0) {
        /* FIXME: this is probably a fatal error */
        goto error;
    }

    /* If no image can be recycled, allocate a new one. */
    if (index1 < 0) {
        /* New image has ident ≥ 1, number = 0 and nrefs = 1 */
        frame_list[index2] = tao_create_2d_shared_array(errs,
                                                        shmcam->pixel_type,
                                                        shmcam->width,
                                                        shmcam->height,
                                                        shmcam->perms);
        index1 = index2;
    }
    return frame_list[index1];

 unlock_and_error:
    UNLOCK_SHARED_CAMERA(errs, shmcam);
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
tao_release_next_frame(tao_error_t** errs, tao_camera_t* cam,
                       tao_shared_array_t* arr)
{
    tao_shared_camera_t* shmcam = cam->shared;
    if (LOCK_SHARED_CAMERA(errs, shmcam) != 0) {
        return -1;
    }

    /* FIXME: check information (pixel size, etc.) */
    /* Increment image number and update image. */
    /* FIXME: timestamp */
    /* FIXME: lock array or assumed it has been locked for the processing? */
    arr->counter = ++shmcam->last_frame.counter;
    arr->nwriters = 0;
    shmcam->last_frame.ident = arr->base.ident;

#if 0
    /* FIXME: Signal that a new image is available to all waiting processes. */
    signalNewImage(shmcam);
#endif

    if (UNLOCK_SHARED_CAMERA(errs, shmcam) != 0) {
        return -1;
    }
    return 0;
}
