/*
 * tao-private.h --
 *
 * Private definitions for compiling TAO.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#ifndef _TAO_PRIVATE_H_
#define _TAO_PRIVATE_H_ 1

#if __STDC_VERSION__ < 199901L
#   error C99 standard must be supported to compile TAO
#   define inline static
#endif

#include <limits.h>
#include <semaphore.h>
#include "tao.h"

#define TAO_ASSERT(expr, code)                                          \
    do {                                                                \
        if (!(expr)) {                                                  \
            tao_push_error(errs, __func__, TAO_ASSERTION_FAILED);       \
            return code;                                                \
        }                                                               \
    } while (false)

/**
 * Get the offset of a member in a structure.
 *
 * @param type    Structure type.
 * @param member  Structure member.
 *
 * @return A number of bytes.
 */
#define TAO_OFFSET_OF(type, member) ((char*)&((type*)0)->member - (char*)0)

/**
 * Round-up `a` by chuncks of size `b`.
 */
#define TAO_ROUND_UP(a, b)  ((((a) + ((b) - 1))/(b))*(b))

/**
 * @def TAO_MAX_SIGNED_INT
 *
 * @brief Yeilds the maximum value of a signed integer.
 *
 * @param T   A signed integer type.
 *
 * @see TAO_MAX_UNSIGNED_INT.
 */
#define TAO_MAX_SIGNED_INT(T) ((((T)1 << (sizeof(T)*CHAR_BIT - 2)) - 1)*2 + 1)
/* CHAR_BIT is defined in <limits.h> */

/**
 * @def TAO_MAX_UNSIGNED_INT
 *
 * @brief Yeilds the maximum value of an unsigned integer.
 *
 * @param T   An unsigned integer type.
 *
 * @see TAO_MAX_SIGNED_INT.
 */
#define TAO_MAX_UNSIGNED_INT(T) (~((T)0))

/**
 * Private structure to store a multi-dimensional array.
 *
 * The definition of this structure is exposed in `"tao-private.h"` but its
 * members should be considered as read-only.  It is recommended to use the
 * public API to manipulate a multi-dimensional array.
 */
struct tao_array {
    int nrefs;                  /**< Number of references on the object */
    int ndims;                  /**< Number of dimensions */
    void* data;                 /**< Address of first array element */
    long nelem;                 /**< Number of elements */
    long dims[TAO_MAX_NDIMS];   /**< Length of each dimension (dimensions
                                     beyong `ndims` are assumed to be `1`) */
    tao_element_type_t  eltype; /**< Type of the elements of the shared
                                     array */
    void (*free)(void*);        /**< If non-NULL, function to call to release
                                     data */
    void* ctx;                  /**< Context used as argument to free */
};

/**
 * Private structure to store a shared array.
 *
 * The definition of this structure is exposed in `"tao-private.h"` but its
 * members should be considered as read-only.  It is recommended to use the
 * public API to manipulate a shared array.
 */
struct tao_shared_array {
    tao_shared_object_t base;     /**< Shared object backing storage of the
                                       shared array */
    size_t offset;                /**< Offset of data part in bytes and
                                       relative to the base address of the
                                       object */
    long nelem;                   /**< Number of elements */
    int ndims;                    /**< Number of dimensions */
    long dims[TAO_MAX_NDIMS];     /**< Length of each dimension (dimensions
                                       beyong `ndims` are assumed to be `1`) */
    tao_element_type_t  eltype;   /**< Type of the elements of the shared
                                       array */
    int32_t nwriters;             /**< Number of writers */
    int32_t nreaders;             /**< Number of readers */
    int64_t counter;              /**< Counter (used for acquired images) */
    int64_t ts_sec;               /**< Time stamp (seconds part) */
    int64_t ts_nsec;              /**< Time stamp (nanoseconds part) */
};

/**
 * Shared camera data.
 *
 * This structure describes the shared data storing the global resources of a
 * camera.  After querying the shared memory identifier to the server (the
 * frame grabber), clients can attach this shared data part with
 * tao_attach_shared_camera().  When a client no longer needs this shared
 * data, it shall call tao_detach_shared_camera().
 *
 * This structure **must** be considered as read-only by the clients and
 * information provided by this structure is only valid as long as the client
 * locks this shared structure by calling tao_lock_shared_camera() and until
 * the client unlock the structure by calling tao_lock_shared_camera().  Beware
 * to not call tao_detach_shared_camera() while the shared data is locked.
 */
struct tao_shared_camera {
    tao_shared_object_t base; /**< Shared object backing storage of the
                                   shared frame grabber */
    sem_t sem[TAO_SHARED_CAMERA_SEMAPHORES];
                     /**< Semaphores used to signal clients that a new image
                          have just been acquired. */
    int           state; /**< State of the camera: 0 if device not yet open, 1
                              if device open but no acquisition is running, 2
                              if acquisition is running. */
    int           depth; /**< Bits per pixel in the raw images. */
    int       pixeltype; /**< Pixel type. */
    int        weighted; /**< Pre-processed images have associated weights. */
    long    sensorwidth; /**< Maximum image width for the detector. */
    long   sensorheight; /**< Maximum image height for the detector. */
    long           xoff; /**< Horizontal offset of the acquired images with
                              respect to the left border of the detector. */
    long           yoff; /**< Vertical offset of the acquired images with
                              respect to the bottom border of the detector. */
    long          width; /**< Number of pixels per line of the acquired
                              images. */
    long         height; /**< Number of lines of pixels in the acquired
                              images. */
    double         bias; /**< Detector bias. */
    double         gain; /**< Detector gain. */
    double    framerate; /**< Acquisition rate in frames per second. */
    double exposuretime; /**< Exposure time in seconds. */
    double        gamma; /**< Gamma correction. */
    struct {
        int32_t   ident; /**< Identifier of the shared array backing the
                              storage of the last image, -1 means unused or
                              invalid. */
        int64_t counter; /**< Counter value of the last image.  It is a unique,
                              monotically increasing number, starting at 1 (0
                              means unused or invalid). */
    }         lastframe; /**< Information relative to the last acquired
                              image. */
};

/**
 * Camera structure for the server.
 *
 * The server have access to the camera data that is shared with its clients
 * plus a list of shared arrays used to store processed frame data as new
 * frames are acquired. The size of this list is at most @a nframes and its
 * contents is recycled if possible.
 */
struct tao_camera {
    tao_shared_camera_t* shared; /**< Attached shared camera data. */
    unsigned              perms; /**< Access permissions for the shared data. */
    int                 nframes; /**< Maximum number of memorized frames. */
    int                   index; /**< Index of current frame. */
    tao_shared_array_t** frames; /**< List of shared arrays. */
    tao_shared_array_t*   spare; /**< A spare array. */
};

#endif /* _TAO_PRIVATE_H_ */
