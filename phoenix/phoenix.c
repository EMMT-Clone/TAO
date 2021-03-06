/*
 * phoenix.c --
 *
 * Interface to ActiveSilicon Phoenix frame grabber.
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

#ifdef _PHX_POSIX
#  include <termios.h>
#endif

#define ALIGNMENT 32
#if defined(__BIGGEST_ALIGNMENT__) && __BIGGEST_ALIGNMENT__ > ALIGNMENT
#  undef ALIGNMENT
#  define ALIGNMENT __BIGGEST_ALIGNMENT__
#endif

#define ROUND_UP(a, b) ((((b) - 1 + (a))/(b))*(b))

#define MIN(a,b) ((a) <= (b) ? (a) : (b))

#define MAX(a,b) ((a) >= (b) ? (a) : (b))

#define NEW(errs, type, numb) ((type*)tao_calloc(errs, (numb), sizeof(type)))

static struct {
    const char* model;
    int (*check)(phx_camera_t* cam);
    int (*initialize)(phx_camera_t* cam);
} known_cameras[] = {
    {"Mikrotron MC408x",
     phx_check_mikrotron_mc408x,
     phx_initialize_mikrotron_mc408x},
    {NULL, NULL, NULL}
};

/*---------------------------------------------------------------------------*/
/* LOCKS */

/*
 * We should not modify anything in the camera instance unless we own the
 * camera lock.  As a consequence errors cannot be safely tracked by means of
 * the camera error stack while locking/unlocking the camera.  We have to
 * assume that any errors occuring during theese operations are fatal.  In
 * fact, any errors occuring while locking/unlocking are an indication of bugs
 * and aborting the program is the best we can do.
 *
 * Note that pthread_cond_init, pthread_cond_signal, pthread_cond_broadcast,
 * and pthread_cond_wait never return an error code.  The
 * pthread_cond_timedwait function may return ETIMEDOUT is the condition
 * variable was not signaled until the specified timeout or EINTR if the call
 * was interrupted by a signal.
 */

void
phx_lock(phx_camera_t* cam)
{
    int code = pthread_mutex_lock(&cam->mutex);
    if (code != 0) {
        /* This is a fatal error. */
        tao_push_error(NULL, "pthread_mutex_lock", code);
    }
}

int
phx_try_lock(phx_camera_t* cam)
{
    int code = pthread_mutex_trylock(&cam->mutex);
    if (code == 0) {
        return 1;
    } else if (code == EBUSY) {
        return 0;
    } else {
        /* This is a fatal error. */
        tao_push_error(NULL, "pthread_mutex_trylock", code);
        return -1;
    }
}

void
phx_unlock(phx_camera_t* cam)
{
    int code = pthread_mutex_unlock(&cam->mutex);
    if (code != 0) {
        /* This is a fatal error. */
        tao_push_error(NULL, "pthread_mutex_unlock", code);
    }
}

void
phx_signal_condition(phx_camera_t* cam)
{
    /* Manual pages say that pthread_cond_signal never returns an error
       code. */
    (void)pthread_cond_signal(&cam->cond);
}

/*---------------------------------------------------------------------------*/
/* ERROR MANAGEMENT */

/* Callback for TAO error management system. */
static void
get_error_details(int code, const char** reason, const char** info)
{
    if (reason != NULL) {
        *reason = phx_status_description(code);
    }
    if (info != NULL) {
        *info = phx_status_identifier(code);
        if (*info != NULL && (*info)[0] == '\0') {
            *info = NULL;
        }
    }
}

void
phx_push_error(tao_error_t** errs, const char* func, int code)
{
    tao_push_other_error(errs, func, code, get_error_details);
}


/* Mutex to protect error handler settings. */
static pthread_mutex_t error_handler_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Verbosity of error handler. */
static int error_handler_level = 1;

int
phx_set_error_handler_verbosity(int level)
{
    int previous;
    if (pthread_mutex_lock(&error_handler_mutex) == 0) {
        previous = error_handler_level;
        error_handler_level = level;
        (void)pthread_mutex_unlock(&error_handler_mutex);
    } else {
        previous = 1;
    }
    return previous;
}

int
phx_get_error_handler_verbosity()
{
    int level = -1;
    if (pthread_mutex_lock(&error_handler_mutex) == 0) {
        level = error_handler_level;
        (void)pthread_mutex_unlock(&error_handler_mutex);
    }
    return level;
}

static void
error_handler(const char* funcname, phx_status_t errcode, const char* reason)
{
    if (errcode != PHX_OK) {
        int level = phx_get_error_handler_verbosity();
        if (level >= 2) {
            if (reason != NULL && reason[0] != '\0') {
                fprintf(stderr, "Function %s failed with code 0x%08x.\n%s\n",
                        funcname, (unsigned int)errcode, reason);
            } else {
                fprintf(stderr, "Function %s failed with code 0x%08x.\n",
                        funcname, (unsigned int)errcode);
            }
        }
    }
}

const char*
phx_status_identifier(phx_status_t status)
{
    switch (status) {
#define CASE(x) case x: return #x;
        CASE(PHX_OK);
        CASE(PHX_ERROR_BAD_HANDLE);
        CASE(PHX_ERROR_BAD_PARAM);
        CASE(PHX_ERROR_BAD_PARAM_VALUE);
        CASE(PHX_ERROR_READ_ONLY_PARAM);
        CASE(PHX_ERROR_OPEN_FAILED);
        CASE(PHX_ERROR_INCOMPATIBLE);
        CASE(PHX_ERROR_HANDSHAKE);
        CASE(PHX_ERROR_INTERNAL_ERROR);
        CASE(PHX_ERROR_OVERFLOW);
        CASE(PHX_ERROR_NOT_IMPLEMENTED);
        CASE(PHX_ERROR_HW_PROBLEM);
        CASE(PHX_ERROR_NOT_SUPPORTED);
        CASE(PHX_ERROR_OUT_OF_RANGE);
        CASE(PHX_ERROR_MALLOC_FAILED);
        CASE(PHX_ERROR_SYSTEM_CALL_FAILED);
        CASE(PHX_ERROR_FILE_OPEN_FAILED);
        CASE(PHX_ERROR_FILE_CLOSE_FAILED);
        CASE(PHX_ERROR_FILE_INVALID);
        CASE(PHX_ERROR_BAD_MEMBER);
        CASE(PHX_ERROR_HW_NOT_CONFIGURED);
        CASE(PHX_ERROR_INVALID_FLASH_PROPERTIES);
        CASE(PHX_ERROR_ACQUISITION_STARTED);
        CASE(PHX_ERROR_INVALID_POINTER);
        CASE(PHX_ERROR_LIB_INCOMPATIBLE);
        CASE(PHX_ERROR_SLAVE_MODE);
        CASE(PHX_ERROR_DISPLAY_CREATE_FAILED);
        CASE(PHX_ERROR_DISPLAY_DESTROY_FAILED);
        CASE(PHX_ERROR_DDRAW_INIT_FAILED);
        CASE(PHX_ERROR_DISPLAY_BUFF_CREATE_FAILED);
        CASE(PHX_ERROR_DISPLAY_BUFF_DESTROY_FAILED);
        CASE(PHX_ERROR_DDRAW_OPERATION_FAILED);
        CASE(PHX_ERROR_WIN32_REGISTRY_ERROR);
        CASE(PHX_ERROR_PROTOCOL_FAILURE);
        CASE(PHX_WARNING_TIMEOUT);
        CASE(PHX_WARNING_FLASH_RECONFIG);
        CASE(PHX_WARNING_ZBT_RECONFIG);
        CASE(PHX_WARNING_NOT_PHX_COM);
        CASE(PHX_WARNING_NO_PHX_BOARD_REGISTERED);
        CASE(PHX_WARNING_TIMEOUT_EXTENDED);
#undef CASE
    default:
        return "";
    }
}

int
phx_any_errors(const phx_camera_t* cam)
{
    return (cam != NULL && cam->errs != TAO_NO_ERRORS);
}

void
phx_report_errors(phx_camera_t* cam)
{
    if (cam != NULL && cam->errs != TAO_NO_ERRORS) {
        tao_report_errors(&cam->errs);
    }
}

void
phx_discard_errors(phx_camera_t* cam)
{
    if (cam != NULL && cam->errs != TAO_NO_ERRORS) {
        tao_discard_errors(&cam->errs);
    }
}

/*--------------------------------------------------------------------------*/
/* BYTYE SWAPPING ROUTINES */

#if 0
static void
swap2(void* ptr)
{
    uint8_t* buf = ptr;
    uint8_t c0 = buf[0];
    uint8_t c1 = buf[1];
    buf[0] = c1;
    buf[1] = c0;
}
#endif

static void
swap4(void* ptr)
{
    uint8_t* buf = ptr;
    uint8_t c0 = buf[0];
    uint8_t c1 = buf[1];
    uint8_t c2 = buf[2];
    uint8_t c3 = buf[3];
    buf[0] = c3;
    buf[1] = c2;
    buf[2] = c1;
    buf[3] = c0;
}

static void
swap8(void* ptr)
{
    uint8_t* buf = ptr;
    uint8_t c0 = buf[0];
    uint8_t c1 = buf[1];
    uint8_t c2 = buf[2];
    uint8_t c3 = buf[3];
    uint8_t c4 = buf[4];
    uint8_t c5 = buf[5];
    uint8_t c6 = buf[6];
    uint8_t c7 = buf[7];
    buf[0] = c7;
    buf[1] = c6;
    buf[2] = c5;
    buf[3] = c4;
    buf[4] = c3;
    buf[5] = c2;
    buf[6] = c1;
    buf[7] = c0;
}

/*--------------------------------------------------------------------------*/
/* IMAGE ACQUISITION */

static void
free_virtual_buffers(phx_camera_t* cam)
{
    cam->nbufs = 0;
    cam->bufsize = 0;
    if (cam->bufs != NULL) {
        /* Free buffers in the reverse order of their allocation so that
           we can be interrupted. */
        phx_imgbuf_t* bufs = cam->bufs;
        int i = 0;
        while (bufs[i].pvContext != NULL) {
            ++i;
        }
        while (--i >= 0) {
            free(bufs[i].pvContext);
        }
        cam->bufs = NULL;
        free(bufs);
    }
}

static int
allocate_virtual_buffers(phx_camera_t* cam, int nbufs)
{
    /* Check arguments. */
    if (nbufs < 2) {
        phx_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
        return -1;
    }
    const tao_image_roi_t* dev_roi = &cam->dev_roi;
    const tao_image_roi_t* usr_roi = &cam->cfg.roi;
    if (dev_roi->xoff < 0 || dev_roi->xoff > usr_roi->xoff ||
        dev_roi->yoff < 0 || dev_roi->yoff > usr_roi->yoff ||
        usr_roi->width < 1 ||
        dev_roi->xoff + dev_roi->width < usr_roi->xoff + usr_roi->width ||
        usr_roi->xoff + usr_roi->width > cam->sensorwidth ||
        usr_roi->height < 1 ||
        dev_roi->xoff + dev_roi->height < usr_roi->xoff + usr_roi->height ||
        usr_roi->yoff + usr_roi->height > cam->sensorheight) {
        phx_push_error(&cam->errs, __func__, TAO_BAD_ROI);
        return -1;
    }

    /*
     * Set active, source and destination regions.  The value of parameter
     * PHX_BUF_DST_XLENGTH is the number of bytes per line of the destination
     * buffer (it must be larger of equal the width of the ROI times the number
     * of bits per pixel rounded up to a number of bytes), the value of
     * PHX_BUF_DST_YLENGTH is the number of lines in the destination buffer (it
     * must be larger or equal PHX_ROI_DST_YOFFSET plus PHX_ROI_YLENGTH.
     *
     * FIXME: PHX_BIT_SHIFT_ALIGN_LSB not defined for PHX_BIT_SHIFT
     */
    phx_value_t active_xlength = dev_roi->width;
    phx_value_t active_ylength = dev_roi->width;
    phx_value_t cam_color      = cam->cam_color;
    phx_value_t cam_depth      = cam->cfg.depth;
    phx_value_t src_xoffset    = usr_roi->xoff - dev_roi->xoff;
    phx_value_t src_yoffset    = usr_roi->yoff - dev_roi->yoff;
    phx_value_t roi_xlength    = usr_roi->width;
    phx_value_t roi_ylength    = usr_roi->height;
    phx_value_t buf_format     = cam->buf_format;
    phx_value_t buf_depth      = phx_capture_format_bits(buf_format);
    phx_value_t buf_xlength    = (roi_xlength*buf_depth + 7)/8;
    phx_value_t buf_ylength    = roi_ylength;
    if (phx_set(cam, PHX_CAM_SRC_COL,             cam_color) != 0 ||
        phx_set(cam, PHX_CAM_SRC_DEPTH,           cam_depth) != 0 ||
        phx_set(cam, PHX_CAM_ACTIVE_XOFFSET,              0) != 0 ||
        phx_set(cam, PHX_CAM_ACTIVE_YOFFSET,              0) != 0 ||
        phx_set(cam, PHX_CAM_ACTIVE_XLENGTH, active_xlength) != 0 ||
        phx_set(cam, PHX_CAM_ACTIVE_YLENGTH, active_ylength) != 0 ||
        phx_set(cam, PHX_CAM_XBINNING,                    1) != 0 ||
        phx_set(cam, PHX_CAM_YBINNING,                    1) != 0 ||
        phx_set(cam, PHX_ACQ_XSUB,               PHX_ACQ_X1) != 0 ||
        phx_set(cam, PHX_ACQ_YSUB,               PHX_ACQ_X1) != 0 ||
        phx_set(cam, PHX_ROI_SRC_XOFFSET,       src_xoffset) != 0 ||
        phx_set(cam, PHX_ROI_SRC_YOFFSET,       src_yoffset) != 0 ||
        phx_set(cam, PHX_ROI_XLENGTH,           roi_xlength) != 0 ||
        phx_set(cam, PHX_ROI_YLENGTH,           roi_ylength) != 0 ||
        phx_set(cam, PHX_ROI_DST_XOFFSET,                 0) != 0 ||
        phx_set(cam, PHX_ROI_DST_YOFFSET,                 0) != 0 ||
        phx_set(cam, PHX_DST_FORMAT,             buf_format) != 0 ||
        phx_set(cam, PHX_BUF_DST_XLENGTH,       buf_xlength) != 0 ||
        phx_set(cam, PHX_BUF_DST_YLENGTH,       buf_ylength) != 0 ||
        phx_set(cam, PHX_BIT_SHIFT,                       0) != 0) {
        return -1;
    }

    /* Get buffer size. */
    size_t bufsize = (size_t)buf_xlength*(size_t)buf_ylength;
    if (cam->bufs == NULL || cam->nbufs != nbufs || cam->bufsize != bufsize) {
        /* Allocate new image buffers */
        size_t offset = ROUND_UP(sizeof(phx_virtual_buffer_t), ALIGNMENT);
        size_t size = offset + bufsize;
        free_virtual_buffers(cam);
        cam->bufs = NEW(&cam->errs, phx_imgbuf_t, nbufs + 1);
        if (cam->bufs == NULL) {
            return -1;
        }
        for (int i = 0; i < nbufs; ++i) {
            phx_virtual_buffer_t* buf = tao_malloc(&cam->errs, size);
            if (buf == NULL) {
                free_virtual_buffers(cam);
                return -1;
            }
            buf->data = (void*)(((uint8_t*)buf) + offset);
            buf->counter = -1;
            buf->ts.tv_sec = 0;
            buf->ts.tv_nsec = 0;
            buf->index = i;
            cam->bufs[i].pvAddress = buf->data;
            cam->bufs[i].pvContext = buf;
        }

        /* Terminate the list of buffers */
        cam->bufs[nbufs].pvAddress = NULL;
        cam->bufs[nbufs].pvContext = NULL;
        cam->nbufs = nbufs;
        cam->bufsize = bufsize;
    }

    /* Instruct Phoenix to use the virtual buffers. */
    if (phx_set_parameter(cam, PHX_DST_PTRS_VIRT, cam->bufs) != 0) {
        free_virtual_buffers(cam); /* FIXME: we can keep the virtual buffers? */
        return -1;
    }
    if (phx_set(cam, (PHX_DST_PTR_TYPE|PHX_CACHE_FLUSH|
                      PHX_FORCE_REWRITE), PHX_DST_PTR_USER_VIRT) != 0) {
        free_virtual_buffers(cam); /* FIXME: we can keep the virtual buffers? */
        return -1;
    }

    return 0;
}

/*
 * Callback for acquisition.
 *
 * FIXME: Handle PHX_INTRPT_FRAME_START and PHX_INTRPT_FRAME_END events
 *        to have a better time-stamp.
 */
static void
acquisition_callback(phx_handle_t handle, uint32_t events, void* data)
{
    phx_camera_t* cam = (phx_camera_t*)data;
    phx_imgbuf_t imgbuf;
    struct timespec ts;

    /* Lock the camera and make minimal sanity check. */
    phx_lock(cam);
    if (cam->handle != handle) {
        tao_push_error(&cam->errs, __func__, TAO_ASSERTION_FAILED);
        goto unlock;
    }

    /* Account for events. */
    if ((PHX_INTRPT_BUFFER_READY & events) != 0) {
        /* A new frame is available.  Take its arrival time stamp. */
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
            tao_push_system_error(&cam->errs, "clock_gettime");
            ts.tv_sec = 0;
            ts.tv_nsec = 0;
        }
        /* Get last captured image buffer. */
        if (phx_read_stream(cam, PHX_BUFFER_GET, &imgbuf) != 0) {
            ++cam->lostframes;
            events &= ~PHX_INTRPT_BUFFER_READY;
        } else {
            /* Store the time-stamp, address and index of the last captured
               image and update counters. */
            phx_virtual_buffer_t* vbuf;
            vbuf = (phx_virtual_buffer_t*)imgbuf.pvContext;
            vbuf->ts.tv_sec = ts.tv_sec;
            vbuf->ts.tv_nsec = ts.tv_nsec;
            vbuf->counter = ++cam->frames;
            ++cam->pending;
            cam->last = vbuf->index;
        }
    }
    if ((PHX_INTRPT_FIFO_OVERFLOW & events) != 0) {
        /* Fifo overflow. */
        ++cam->frames;
        ++cam->overflows;
    }
    if ((PHX_INTRPT_SYNC_LOST & events) != 0) {
        /* Synchronization lost. */
        ++cam->frames;
        ++cam->lostsyncs;
    }
    if ((PHX_INTRPT_FRAME_LOST & events) != 0) {
        /* Frame lost. */
        ++cam->frames;
        ++cam->lostframes;
    }
    if (cam->quitting || (cam->events & events) != 0) {
        /* Signal condition for waiting thread. */
        phx_signal_condition(cam);
    }

    /* Unlock the context. */
 unlock:
    phx_unlock(cam);
}

int
phx_start(phx_camera_t* cam, int nbufs)
{
    /*
     * Assume failure because any early returns mean something wrong happens.
     */
    int status = -1;

    /* Minimal checks. */
    if (cam == NULL) {
        return status;
    }

    /* Lock the camera and check state. */
    phx_lock(cam);
    if (cam->state != 1) {
        tao_push_error(&cam->errs, __func__,
                       (cam->state == 0 ? TAO_NOT_READY :
                        (cam->state == 2 ? TAO_ACQUISITION_RUNNING :
                         TAO_CORRUPTED)));
        goto unlock;
    }

    /* Allocate virtual buffers. */
    if (allocate_virtual_buffers(cam, nbufs) != 0) {
        goto unlock;;
    }

    /*
     * Instruct Phoenix to use the virtual buffers.  Note that obsolete (but
     * still used in the examples and the documentation) parameter
     * PHX_ACQ_NUM_IMAGES is the same as PHX_ACQ_NUM_BUFFERS.  FIXME: Following
     * the examples in the documentation, the PHX_FORCE_REWRITE flag is set but
     * its effects are not documented and I am therefore not sure whether this
     * is really needed.
     */
    if (phx_set(cam,  PHX_ACQ_IMAGES_PER_BUFFER,              1) != 0 ||
        phx_set(cam,  PHX_ACQ_BUFFER_START,                   1) != 0 ||
        phx_set(cam,  PHX_ACQ_NUM_BUFFERS,           cam->nbufs) != 0 ||
        phx_set_parameter(cam,  PHX_DST_PTRS_VIRT,    cam->bufs) != 0 ||
        phx_set(cam, (PHX_DST_PTR_TYPE|PHX_CACHE_FLUSH|
                      PHX_FORCE_REWRITE), PHX_DST_PTR_USER_VIRT) != 0) {
        goto unlock;;
    }

    /*
     * Configure frame grabber for continuous acquisition and enable interrupts
     * for expected events.
     */
    phx_value_t events = (PHX_INTRPT_BUFFER_READY  |
                          PHX_INTRPT_FIFO_OVERFLOW |
                          PHX_INTRPT_FRAME_LOST    |
                          PHX_INTRPT_SYNC_LOST);
    if (phx_set(cam, PHX_INTRPT_CLR,                 ~(phx_value_t)0) != 0 ||
        phx_set(cam, PHX_INTRPT_SET, PHX_INTRPT_GLOBAL_ENABLE|events) != 0 ||
        phx_set(cam, PHX_ACQ_BLOCKING,                    PHX_ENABLE) != 0 ||
        phx_set(cam, PHX_ACQ_CONTINUOUS,                  PHX_ENABLE) != 0 ||
        phx_set(cam, PHX_COUNT_BUFFER_READY,                       1) != 0) {
        goto unlock;;
    }
    cam->events = events;

    /* Setup callback context. */
    if (phx_set_parameter(cam, PHX_EVENT_CONTEXT, (void*)cam) != 0) {
        goto unlock;;
    }

    /* Start acquisition with given callback. */
    if (phx_read_stream(cam, PHX_START, acquisition_callback) != 0) {
        goto unlock;;
    }

    /* Send specific start command. */
    if (cam->start != NULL && cam->start(cam) != 0) {
        phx_read_stream(cam, PHX_ABORT, NULL);
        phx_read_stream(cam, PHX_UNLOCK, NULL);
        goto unlock;;
    }

    /*
     * Update state and quitting status.  Then change status to reflect
     * success.
     */
    cam->state = 2;
    cam->quitting = false;
    status = 0;

    /* Unlock the camera and return status. */
 unlock:
    phx_unlock(cam);
    return status;
}

/*
 * FIXME: Something not specified in the doc. is that, when continuous
 * acquisition and blocking mode are both enabled, all calls to `PHX_BUFFER_GET`
 * yield the same image buffer until `PHX_BUFFER_RELEASE` is called.  It seems
 * that there is no needs to have a `PHX_BUFFER_GET` matches a
 * `PHX_BUFFER_RELEASE` and that every `PHX_BUFFER_RELEASE` moves to the next
 * buffer.  However, acquisition buffers are used in their given order so it is
 * not too difficult to figure out where we are if we count the number of
 * frames.
 */

int
phx_release_buffer(phx_camera_t* cam)
{
    int status;

    if (cam == NULL) {
        return -1;
    }
    phx_lock(cam);
    {
        if (cam->pending > 0) {
            --cam->pending;
            status = phx_read_stream(cam, PHX_BUFFER_RELEASE, NULL);
        } else {
            status = -1;
        }
    }
    phx_unlock(cam);
    return status;
}

int
phx_wait(phx_camera_t* cam, double secs, bool drop)
{
    struct timespec ts;
    bool forever;
    int code;

    /*
     * Lock camera and check state.  Assume failure (index = -1) because any
     * early returns mean that something wrong happens.
     */
    int index = -1;
    if (cam == NULL) {
        return index;
    }
    phx_lock(cam);
    if (cam->state != 2) {
        tao_push_error(&cam->errs, __func__,
                       ((cam->state == 0 || cam->state == 1) ?
                        TAO_NO_ACQUISITION : TAO_CORRUPTED));
        goto unlock;
    }

    /* Compute absolute time for time-out. */
    if (isnan(secs) || secs < 0) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
        goto unlock;
    }
    if (secs > TAO_YEAR) {
        forever = true;
    } else {
        if (tao_get_absolute_timeout(&cam->errs, &ts, secs) != 0) {
            goto unlock;
        }
        forever = ! tao_is_finite_absolute_time(&ts);
    }

    /*
     * While there are no pending frames, wait for the condition to be signaled
     * or an error to occur.  This is done in a `while` loop to cope with
     * spurious signaled conditions.
     */
    while (! cam->quitting && cam->pending < 1) {
        if (forever) {
            code = pthread_cond_wait(&cam->cond, &cam->mutex);
            if (code != 0) {
                tao_push_error(&cam->errs, "pthread_cond_wait", code);
                goto unlock;
            }
        } else {
            code = pthread_cond_timedwait(&cam->cond, &cam->mutex, &ts);
            if (code != 0) {
                if (code == ETIMEDOUT) {
                    index = 0;
                } else {
                    tao_push_error(&cam->errs, "pthread_cond_timedwait", code);
                }
                goto unlock;
            }
        }
    }

    if (! cam->quitting) {
        /*
         * If requested, get rid of unprocessed pending image buffers.
         * FIXME: Do this in the acquisition callback.
         */
        if (drop) {
            while (cam->pending > 1) {
                ++cam->overflows;
                if (phx_release_buffer(cam) != 0) {
                    goto unlock;
                }
            }
        }

        /*
         * At least one image buffer is unprocessed, manage to return the
         * 1-based index of this buffer.
         */
        int k = (cam->last + 1 - cam->pending)%cam->nbufs;
        index = k + (k >= 0 ? 1 : 1 + cam->nbufs);
    }

    /* Unlock the mutex (whatever the errors so far). */
 unlock:
    phx_unlock(cam);
    return index;
}

static int
stop_acquisition(phx_camera_t* cam, phx_acquisition_t command)
{
    /* Minimal checks. */
    if (cam == NULL) {
        return -1;
    }

    /* Assume success and lock the camera. */
    int status = 0;
    phx_lock(cam);
    if (cam->state != 2) {
        tao_push_error(&cam->errs,
                       (command == PHX_STOP ? "phx_stop" : "phx_abort"),
                       (cam->state < 2 ? TAO_NO_ACQUISITION : TAO_CORRUPTED));
        status = -1;
        goto unlock;
    }

    /* Stop/abort acquisition. */
    if (phx_read_stream(cam, command, NULL) != 0) {
        status = -1;
    }

    /* Unlock all buffers */
    if (phx_read_stream(cam, PHX_UNLOCK, NULL) != 0) {
        status = -1;
    }

    /* Call specific stop hook. */
    if (cam->stop != NULL && cam->stop(cam) != 0) {
        status = -1;
    }

    /* Change camera state and signal the end of the acquisition to others. */
    cam->state = 1;
    cam->quitting = true;
    phx_signal_condition(cam);

 unlock:
    phx_unlock(cam);
    return status;
}

int
phx_stop(phx_camera_t* cam)
{
    return stop_acquisition(cam, PHX_STOP);
}

int
phx_abort(phx_camera_t* cam)
{
    return stop_acquisition(cam, PHX_ABORT);
}

/*--------------------------------------------------------------------------*/
/* CREATE/DESTROY/CONFIGURE CAMERA INSTANCE */

static int
check_coaxpress(phx_camera_t* cam)
{
    phx_value_t info;
    uint32_t magic, width, height, pixel_format;

    if (phx_get_parameter(cam, PHX_CXP_INFO, &info) != 0) {
        return -1;
    }
    cam->coaxpress = false;
    if ((info & PHX_CXP_CAMERA_DISCOVERED) != 0) {
        /*
         * The frame grabber thinks that we have a CoaXPress camera.  We check
         * the magic number and set the byte order for CoaXPress communication.
         */
        if (cxp_get(cam, STANDARD, &magic) != 0) {
        bad_magic:
            tao_push_error(&cam->errs, __func__, TAO_BAD_MAGIC);
            return -1;
        }
        if (magic != CXP_MAGIC) {
            swap4(&magic);
            if (magic != CXP_MAGIC) {
                goto bad_magic;
            }
            cam->swap = ! cam->swap;
        }
        cam->coaxpress = true;

        /*
         * Get pixel format, current image size (full width and full height must
         * be correctly set later), device vendor name and device model name.
         */
        if (cxp_get(cam, PIXEL_FORMAT_ADDRESS, &pixel_format) != 0 ||
            cxp_get(cam, WIDTH_ADDRESS,               &width) != 0 ||
            cxp_get(cam, HEIGHT_ADDRESS,             &height) != 0 ||
            cxp_get(cam, DEVICE_VENDOR_NAME,     cam->vendor) != 0 ||
            cxp_get(cam, DEVICE_MODEL_NAME,       cam->model) != 0) {
            return -1;
        }
        cam->pixel_format    = pixel_format;
        cam->cfg.roi.xoff    = 0;
        cam->cfg.roi.yoff    = 0;
        cam->cfg.roi.width   = width;
        cam->cfg.roi.height  = height;
        cam->sensorwidth     = width;
        cam->sensorheight    = height;
        memcpy(&cam->dev_roi, &cam->cfg.roi, sizeof(tao_image_roi_t));
    }
    return 0;
}

phx_camera_t*
phx_create(tao_error_t** errs,
           void (*handler)(const char*, phx_status_t, const char*),
           char* configname, phx_value_t boardnumber)
{
    phx_camera_t* cam;
    phx_status_t status;
    int code;

    /* Check assumptions. */
    assert(sizeof(ui8)  == 1);
    assert(sizeof(ui16) == 2);
    assert(sizeof(ui32) == 4);
    assert(sizeof(ui64) == 8);
    assert(sizeof(float32_t) == 4);
    assert(sizeof(float64_t) == 8);
    assert(sizeof(etParamValue) == sizeof(int));

    /* Allocate memory to store the camera instance. */
    cam = NEW(errs, phx_camera_t, 1);
    if (cam == NULL) {
        return NULL;
    }
    cam->timeout = 500;
    cam->state = -3;
    cam->swap = false;
    cam->coaxpress = false;

    /* Initialize lock. */
    code = pthread_mutex_init(&cam->mutex, NULL);
    if (code != 0) {
        tao_push_error(&cam->errs, "pthread_mutex_init", code);
        goto error;
    }
    cam->state = -2;

    /* Initialize condition variable. */
    code = pthread_cond_init(&cam->cond, NULL);
    if (code != 0) {
        tao_push_error(&cam->errs, "pthread_cond_init", code);
        goto error;
    }
    cam->state = -1;

    /* Create a Phoenix handle. */
    if (handler == NULL) {
        handler = error_handler;
    }
    status = PHX_Create(&cam->handle, handler);
    if (status != PHX_OK) {
       phx_push_error(errs, "PHX_Create", status);
       goto error;
    }
    cam->state = 0;

    /* Set the configuration file name. */
    if (configname != NULL && configname[0] != 0) {
        status = PHX_ParameterSet(cam->handle, PHX_CONFIG_FILE, &configname);
        if (status != PHX_OK) {
            phx_push_error(errs, "PHX_ParameterSet", status);
            goto error;
        }
    }

    /* Set the board number. */
    status = PHX_ParameterSet(cam->handle, PHX_BOARD_NUMBER, &boardnumber);
    if (status != PHX_OK) {
        phx_push_error(errs, "PHX_ParameterSet", status);
        goto error;
    }

    /* Open the Phoenix board using the above configuration file. */
    status = PHX_Open(cam->handle);
    if (status != PHX_OK) {
        phx_push_error(errs, "PHX_Open", status);
        goto error;
    }
    cam->state = 1;

    /* Check whether we have a CoaXPress camera. */
    if (check_coaxpress(cam) != 0) {
        goto error;
    }

    /* Initialize specific camera model. */
    for (int k = 0; known_cameras[k].check != NULL; ++k) {
        if (known_cameras[k].check(cam) &&
            known_cameras[k].initialize != NULL) {
            if (known_cameras[k].initialize(cam) != 0) {
                goto error;
            }
            cam->initialize = known_cameras[k].initialize;
            break;
        }
    }

    return cam;

 error:
    if (cam != NULL) {
        tao_transfer_errors(errs, &cam->errs);
    }
    phx_destroy(cam);
    return NULL;
}

void
phx_destroy(phx_camera_t* cam)
{
    if (cam != NULL) {
        /* Close/release/destroy resources in reverse order. */
        if (cam->state >= 2) {
            /* Abort acquisition and unlock all buffers. */
            (void)PHX_StreamRead(cam->handle, PHX_ABORT, NULL);
            (void)PHX_StreamRead(cam->handle, PHX_UNLOCK, NULL);
            if (cam->stop != NULL) {
                (void)cam->stop(cam);
            }
            cam->state = 1;
        }
        if (cam->state >= 1) {
            /* Close the Phoenix board. */
            (void)PHX_Close(&cam->handle);
            cam->state = 0;
        }
        if (cam->state >= 0) {
            /* Destroy the Phoenix handle and release memory. */
            (void)PHX_Destroy(&cam->handle);
        }
        if (cam->state >= -1) {
            /* Destroy condition variable. */
            pthread_cond_destroy(&cam->cond);
        }
        if (cam->state >= -2) {
            /* Destroy mutex. */
            pthread_mutex_destroy(&cam->mutex);
        }

        /* Free virtual buffers. */
        free_virtual_buffers(cam);

        /* Free error stack. */
        tao_discard_errors(&cam->errs);

        /* Release memory. */
        free(cam);
    }
}

int
phx_load_configuration(phx_camera_t* cam, int id)
{
    int status = -1;
    if (cam == NULL) {
        errno = EFAULT;
    } else {
        phx_lock(cam);
        if (cam->state >= 2) {
            tao_push_error(&cam->errs, __func__, TAO_ACQUISITION_RUNNING);
        } else if (cam->load_config == NULL) {
            tao_push_error(&cam->errs, __func__, TAO_UNSUPPORTED);
        } else {
            status = cam->load_config(cam, id);
        }
        phx_unlock(cam);
    }
    return status;
}

int
phx_save_configuration(phx_camera_t* cam, int id)
{
    int status = -1;
    if (cam == NULL) {
        errno = EFAULT;
    } else {
        phx_lock(cam);
        if (cam->state >= 2) {
            tao_push_error(&cam->errs, __func__, TAO_ACQUISITION_RUNNING);
        } else if (cam->save_config == NULL) {
            tao_push_error(&cam->errs, __func__, TAO_UNSUPPORTED);
        } else {
            status = cam->save_config(cam, id);
        }
        phx_unlock(cam);
    }
    return status;
}

void
phx_get_configuration(phx_camera_t* cam, phx_config_t* cfg)
{
    if (cfg != NULL) {
        if (cam == NULL) {
            memset(cfg, 0, sizeof(phx_config_t));
        } else {
            phx_lock(cam);
            memcpy(cfg, &cam->cfg, sizeof(phx_config_t));
            phx_unlock(cam);
        }
    }
}

int
phx_set_configuration(phx_camera_t* cam, const phx_config_t* cfg)
{
    int status = -1;
    if (cam == NULL) {
        errno = EFAULT;
    } else if (cfg == NULL) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ADDRESS);
    } else if (cfg->depth <= 0) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_DEPTH);
    } else if (isnan(cfg->bias) || isinf(cfg->bias)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_BIAS);
    } else if (isnan(cfg->gain) || isinf(cfg->gain)) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_GAIN);
    } else if (isnan(cfg->exposuretime) || isinf(cfg->exposuretime) ||
        cfg->exposuretime < 0) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_EXPOSURETIME);
    } else if (isnan(cfg->framerate) || isinf(cfg->framerate) || cfg->framerate <= 0) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_FRAMERATE);
    } else if (cfg->roi.xoff < 0 || cfg->roi.yoff < 0 ||
        cfg->roi.width < 1 || cfg->roi.height < 1 ||
        cfg->roi.xoff + cfg->roi.width > cam->sensorwidth ||
        cfg->roi.yoff + cfg->roi.height > cam->sensorheight) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ROI);
    } else {
        phx_lock(cam);
        if (cam->state >= 2) {
            tao_push_error(&cam->errs, __func__, TAO_ACQUISITION_RUNNING);
        } else if (cam->set_config == NULL) {
            tao_push_error(&cam->errs, __func__, TAO_UNSUPPORTED);
        } else {
            status = cam->set_config(cam, cfg);
        }
        phx_unlock(cam);
    }
    return status;
}

int
phx_update_configuration(phx_camera_t* cam)
{
    int status = -1;
    if (cam == NULL) {
        errno = EFAULT;
    } else {
        phx_lock(cam);
        if (cam->state >= 2) {
            tao_push_error(&cam->errs, __func__, TAO_ACQUISITION_RUNNING);
        } else if (cam->update_config == NULL) {
            tao_push_error(&cam->errs, __func__, TAO_UNSUPPORTED);
        } else {
            status = cam->update_config(cam);
        }
        phx_unlock(cam);
    }
    return status;
}

int
phx_update_temperature(phx_camera_t* cam)
{
    int status = -1;
    if (cam == NULL) {
        errno = EFAULT;
    } else {
        phx_lock(cam);
        /* FIXME: Perhaps this can be done while acquisition is running. */
        if (cam->state >= 2) {
            tao_push_error(&cam->errs, __func__, TAO_ACQUISITION_RUNNING);
        } else if (cam->update_temperature == NULL) {
            tao_push_error(&cam->errs, __func__, TAO_UNSUPPORTED);
        } else {
            status = cam->update_temperature(cam);
        }
        phx_unlock(cam);
    }
    return status;
}


/*--------------------------------------------------------------------------*/
/* WRAPPERS FOR PHOENIX ROUTINES */

int
phx_read_stream(phx_camera_t* cam, phx_acquisition_t command, void* addr)
{
   phx_status_t status = PHX_StreamRead(cam->handle, command, addr);
   if (status != PHX_OK) {
       if (command == PHX_UNLOCK && status == PHX_ERROR_NOT_IMPLEMENTED) {
           /* Ignore this error. */
           return 0;
       }
       phx_push_error(&cam->errs, "PHX_StreamRead", status);
       return -1;
   }
   return 0;
}

/*---------------------------------------------------------------------------*/
/* ROUTINES TO QUERY/SET FRAME GRABBER PARAMETERS */

int
phx_get_parameter(phx_camera_t* cam, phx_param_t param, void* addr)
{
   phx_status_t status = PHX_ParameterGet(cam->handle, param, addr);
   if (status != PHX_OK) {
       phx_push_error(&cam->errs, "PHX_ParameterGet", status);
       return -1;
   }
   return 0;
}

int
phx_set_parameter(phx_camera_t* cam, phx_param_t param, void* addr)
{
   phx_status_t status = PHX_ParameterSet(cam->handle, param, addr);
   if (status != PHX_OK) {
       phx_push_error(&cam->errs, "PHX_ParameterSet", status);
       return -1;
   }
   return 0;
}

int
phx_get(phx_camera_t* cam, phx_param_t param, phx_value_t* valptr)
{
    return phx_get_parameter(cam, param, valptr);
}

int
phx_set(phx_camera_t* cam, phx_param_t param, phx_value_t value)
{
    return phx_set_parameter(cam, param, &value);
}


/*---------------------------------------------------------------------------*/
/* ROUTINES TO READ/WRITE COAXPRESS REGISTERS */

int
cxp_read(phx_camera_t* cam, uint32_t addr, uint8_t* data, uint32_t* size)
{
    phx_status_t status = PHX_ControlRead(cam->handle, PHX_REGISTER_DEVICE,
                                          &addr, data, size, cam->timeout);
    if (status != PHX_OK) {
        phx_push_error(&cam->errs, "PHX_ControlRead", status);
        return -1;
    }
    return 0;
}

/*
 * Unfortunately, setting some parameters (as the pixel format or the gamma
 * correction) returns an error with an absurd code (PHX_ERROR_MALLOC_FAILED)
 * even if the value has been correctly set.  The error cannot be just ignored
 * as further reads of registers yield wrong values.  The strategy is to close
 * and re-open the camera when such an error occurs which solves the problem in
 * practice to the cost of the time spent to close and re-open (0.4 sec.).  To
 * avoid alarming the user, printing of error messages is disabled during this
 * process.
 *
 * Other (unsuccessful) strategies have been tried:
 *
 * - Calling PHX_ControlReset yields a PHX_ERROR_BAD_HANDLE error.
 *
 * - Re-reading the register until its value is OK (this does not work for
 *   write-only registers).
 */
int
cxp_write(phx_camera_t* cam, uint32_t addr, uint8_t* data, uint32_t* size)
{
    int result = 0;
    int level = phx_set_error_handler_verbosity(0);
    phx_status_t status = PHX_ControlWrite(cam->handle, PHX_REGISTER_DEVICE,
                                           &addr, data, size, cam->timeout);
    if (status == PHX_ERROR_MALLOC_FAILED) {
        /* This is probably a bogus register. */
        status = PHX_Close(&cam->handle);
        cam->state = 0;
        if (status != PHX_OK) {
            phx_push_error(&cam->errs, "PHX_Close", status);
            result = -1;
        }
        if (result == 0) {
            status = PHX_Open(cam->handle);
            if (status != PHX_OK) {
                phx_push_error(&cam->errs, "PHX_Open", status);
                result = -1;
            } else {
                cam->state = 1;
            }
        }
        if (result == 0) {
            if (cam->initialize != NULL && cam->initialize(cam) != 0) {
                result = -1;
            }
        }
        phx_set_error_handler_verbosity(level);
    } else if (status != PHX_OK) {
        phx_push_error(&cam->errs, "PHX_ControlWrite", status);
        result = -1;
    }
    return result;
}

int
cxp_reset(phx_camera_t* cam, uint32_t addr)
{
    phx_status_t status = PHX_ControlReset(cam->handle, PHX_REGISTER_DEVICE,
                                           &addr, cam->timeout);
    if (status != PHX_OK) {
        phx_push_error(&cam->errs, "PHX_ControlReset", status);
        return -1;
    }
    return 0;
}

#define FUNCTIONS(S,T,N)                                                \
    int                                                                 \
    cxp_read_##S(phx_camera_t* cam, uint32_t addr, T* value)            \
    {                                                                   \
        uint32_t size = sizeof(T);                                      \
        int status = cxp_read(cam, addr, (uint8_t*)value, &size);       \
        if (status == 0 && cam->swap) {                                 \
            swap##N(value);                                             \
        }                                                               \
        return status;                                                  \
    }                                                                   \
                                                                        \
    int                                                                 \
    cxp_write_##S(phx_camera_t* cam, uint32_t addr, T value)            \
    {                                                                   \
        uint32_t size = sizeof(T);                                      \
        if (cam->swap) {                                                \
            swap##N(&value);                                            \
        }                                                               \
        return cxp_write(cam, addr, (uint8_t*)&value, &size);           \
    }
FUNCTIONS(uint32,  uint32_t,  4)
FUNCTIONS(uint64,  uint64_t,  8)
FUNCTIONS(float32, float32_t, 4)
FUNCTIONS(float64, float64_t, 8)
#undef FUNCTIONS

int
cxp_read_string(phx_camera_t* cam, uint32_t addr, uint32_t len, char* buf)
{
    uint32_t count = len;
    if (cxp_read(cam, addr, (uint8_t*)buf, &count) != 0) {
        return -1;
    }
    buf[MIN(len, count)] = '\0';
    return 0;
}

int
cxp_read_indirect_uint32(phx_camera_t* cam, uint32_t addr, uint32_t* value)
{
    uint32_t regaddr;

    /* Get the address of the register. */
    if (cxp_read_uint32(cam, addr, &regaddr) != 0) {
        return -1;
    }

    /* Get the value at that address. */
    return cxp_read_uint32(cam, regaddr, value);
}

/*--------------------------------------------------------------------------*/
/* UTILITIES */

static int
print_board_info(phx_camera_t* cam, const char* pfx, FILE* stream);

int
phx_print_camera_info(phx_camera_t* cam, FILE* stream)
{
    /* Minimal checks and set defaults. */
    if (cam == NULL) {
        errno = EFAULT;
        return -1;
    }
    if (stream == NULL) {
        stream = stdout;
    }

    /* Assume success and lock the camera. */
    int status = 0;
    phx_lock(cam);

    /* Print information and settings. */
    fprintf(stream, "Camera vendor: %s\n",
            (cam->vendor[0] != '\0' ? cam->vendor : "Unknown"));
    fprintf(stream, "Camera model: %s\n",
            (cam->model[0] != '\0' ? cam->model : "Unknown"));
    fprintf(stream, "CoaXPress camera: %s\n",
            (cam->coaxpress ? "yes" : "no"));
    fprintf(stream, "Board information:\n");
    if (print_board_info(cam, "    ", stream) != 0) {
        status = -1;
    }
    fprintf(stream, "Connection channels: %u\n",
            cam->cfg.connection.channels);
    fprintf(stream, "Connection speed: %u Mbps\n",
            cam->cfg.connection.speed);
    fprintf(stream, "Bits per pixel: %d\n", (int)cam->cfg.depth);
    fprintf(stream, "Sensor size: %d × %d pixels\n",
            (int)cam->sensorwidth,  (int)cam->sensorheight);
    fprintf(stream, "Region of interest: %d × %d at (%d,%d)\n",
            (int)cam->cfg.roi.width,  (int)cam->cfg.roi.height,
            (int)cam->cfg.roi.xoff, (int)cam->cfg.roi.yoff);
    fprintf(stream, "Active region:      %d × %d at (%d,%d)\n",
            (int)cam->dev_roi.width,  (int)cam->dev_roi.height,
            (int)cam->dev_roi.xoff, (int)cam->dev_roi.yoff);
    fprintf(stream, "Detector bias: %5.1f\n", cam->cfg.bias);
    fprintf(stream, "Detector gain: %5.1f\n", cam->cfg.gain);
    fprintf(stream, "Exposure time: %g s\n", cam->cfg.exposuretime);
    fprintf(stream, "Frame framerate: %.1f Hz\n", cam->cfg.framerate);
    if (cam->update_temperature != NULL) {
        if (cam->update_temperature(cam) == 0) {
            fprintf(stream, "Detector temperature: %.1f °C\n",
                    cam->temperature);
        } else {
            status = -1;
        }
    }

    /* Unlock camera and return status. */
    phx_unlock(cam);
    return status;
}

static int
print_board_info(phx_camera_t* cam, const char* pfx, FILE* stream)
{
    /* Retrieve information. */
    phx_value_t bits;
     if (phx_get(cam, PHX_BOARD_INFO, &bits) != 0) {
        return -1;
    }
#define PRT(msk, txt)                                   \
    do {                                                \
        if ((bits & (msk)) == (msk)) {                  \
            fprintf(stream, "%s%s\n", pfx, txt);        \
        }                                               \
    } while (false)
#define CASE(cst, txt) case cst: fprintf(stream, "%s%s\n", pfx, txt); break
#define CASE1(cst) CASE(cst, #cst)

    PRT(PHX_BOARD_INFO_LVDS, "Board has LVDS camera interface");
    PRT(PHX_BOARD_INFO_CL, "Board has Camera Link interface");
    PRT(PHX_BOARD_INFO_CL_BASE,
        "Board is using Camera Link Base interface only");
    PRT(PHX_BOARD_INFO_CL_MEDIUM,
        "Board is using Camera Link Medium interface");
    PRT(PHX_BOARD_INFO_CL_FULL, "Board is using Camera Link Full interface");
    PRT(PHX_BOARD_INFO_PCI_EXPRESS, "Board has PCI Express interface");
    PRT(PHX_BOARD_INFO_PCI_3V, "3V PCI Interface");
    PRT(PHX_BOARD_INFO_PCI_5V, "5V PCI Interface");
    PRT(PHX_BOARD_INFO_PCI_33M, "33MHz PCI Interface");
    PRT(PHX_BOARD_INFO_PCI_66M, "66MHz PCI Interface");
    PRT(PHX_BOARD_INFO_PCI_32B, "32bit PCI Interface");
    PRT(PHX_BOARD_INFO_PCI_64B, "64bit PCI Interface");
    PRT(PHX_BOARD_INFO_BOARD_3V, "Board is 3V compatible");
    PRT(PHX_BOARD_INFO_BOARD_5V, "Board is 5V compatible");
    PRT(PHX_BOARD_INFO_BOARD_33M, "Board is 33MHz compatible");
    PRT(PHX_BOARD_INFO_BOARD_66M, "Board is 66MHz compatible");
    PRT(PHX_BOARD_INFO_BOARD_32B, "Board is 32bit compatible");
    PRT(PHX_BOARD_INFO_BOARD_64B, "Board is 64bit compatible");
    PRT(PHX_BOARD_INFO_CHAIN_MASTER, "Board has chaining jumper set to Master");
    PRT(PHX_BOARD_INFO_CHAIN_SLAVE, "Board has chaining jumper set to Slave");

    if ((bits & PHX_BOARD_INFO_PCI_EXPRESS) != PHX_BOARD_INFO_PCI_EXPRESS) {
        phx_value_t val;
        if (phx_get(cam, PHX_PCIE_INFO, &val) != 0) {
            return -1;
        }
        switch ((int)(val & PHX_EMASK_PCIE_INFO_LINK_GEN)) {
            CASE(PHX_PCIE_INFO_LINK_GEN1,
                 "PCI Express link operating at Gen 1 PCI Express speed");
            CASE(PHX_PCIE_INFO_LINK_GEN2,
                 "PCI Express link operating at Gen 2 PCI Express speed");
            CASE(PHX_PCIE_INFO_LINK_GEN3,
                 "PCI Express link operating at Gen 3 PCI Express speed");
        default:
            fprintf(stream, "%sUnknown PCI Express link generation\n", pfx);
        }
        switch ((int)(val & PHX_EMASK_PCIE_INFO_LINK_X)) {
            CASE(PHX_PCIE_INFO_LINK_X1,
                 "PCI Express link operating at x1 PCI Express width");
            CASE(PHX_PCIE_INFO_LINK_X2,
                 "PCI Express link operating at x2 PCI Express width");
            CASE(PHX_PCIE_INFO_LINK_X4,
                 "PCI Express link operating at x4 PCI Express width");
            CASE(PHX_PCIE_INFO_LINK_X8,
                 "PCI Express link operating at x8 PCI Express width");
            CASE(PHX_PCIE_INFO_LINK_X12,
                 "PCI Express link operating at x12 PCI Express width");
            CASE(PHX_PCIE_INFO_LINK_X16,
                 "PCI Express link operating at x16 PCI Express width");
            CASE(PHX_PCIE_INFO_LINK_X32,
                 "PCI Express link operating at x32 PCI Express width");
        default:
            fprintf(stream, "%sUnknown PCI Express link width\n", pfx);
        }
        switch ((int)(val & PHX_EMASK_PCIE_INFO_FG_GEN)) {
            CASE(PHX_PCIE_INFO_FG_GEN1,
                 "Frame grabber only supports Gen 1 PCI Express");
            CASE(PHX_PCIE_INFO_FG_GEN2,
                 "Frame grabber supports Gen 2 PCI Express");
            CASE(PHX_PCIE_INFO_FG_GEN3,
                 "Frame grabber supports Gen 3 PCI Express");
        }
        switch ((int)(val & PHX_EMASK_PCIE_INFO_FG_X)) {
            CASE(PHX_PCIE_INFO_FG_X1,  "Frame grabber x1");
            CASE(PHX_PCIE_INFO_FG_X2,  "Frame grabber x2");
            CASE(PHX_PCIE_INFO_FG_X4,  "Frame grabber x4");
            CASE(PHX_PCIE_INFO_FG_X8,  "Frame grabber x8");
            CASE(PHX_PCIE_INFO_FG_X12, "Frame grabber x12");
            CASE(PHX_PCIE_INFO_FG_X16, "Frame grabber x16");
            CASE(PHX_PCIE_INFO_FG_X32, "Frame grabber x32");
        }
        switch ((int)(val & PHX_EMASK_PCIE_INFO_SLOT_GEN)) {
            CASE(PHX_PCIE_INFO_SLOT_GEN1, "Slot Gen1");
            CASE(PHX_PCIE_INFO_SLOT_GEN2, "Slot Gen2");
            CASE(PHX_PCIE_INFO_SLOT_GEN3, "Slot Gen3");
        }
        switch ((int)(val & PHX_EMASK_PCIE_INFO_SLOT_X)) {
            CASE(PHX_PCIE_INFO_SLOT_X1,  "Slot x1");
            CASE(PHX_PCIE_INFO_SLOT_X2,  "Slot x2");
            CASE(PHX_PCIE_INFO_SLOT_X4,  "Slot x4");
            CASE(PHX_PCIE_INFO_SLOT_X8,  "Slot x8");
            CASE(PHX_PCIE_INFO_SLOT_X12, "Slot x12");
            CASE(PHX_PCIE_INFO_SLOT_X16, "Slot x16");
            CASE(PHX_PCIE_INFO_SLOT_X32, "Slot x32");
        }
    }

    /* CoaXPress information. */
    if (phx_get(cam, PHX_CXP_INFO, &bits) != 0) {
        return -1;
    }
    PRT(PHX_CXP_CAMERA_DISCOVERED,
        "The CoaXPress camera has completed discovery");
    PRT(PHX_CXP_CAMERA_IS_POCXP,
        "The CoaXPress camera is powered via PoCXP from the frame grabber");
    PRT(PHX_CXP_POCXP_UNAVAILABLE,
        "There is no power to the frame grabber to provide PoCXP to a camera");
    PRT(PHX_CXP_POCXP_TRIPPED,
        "The PoCXP supply to the camera has been shutdown because high "
        "current was detected");
    int links[4], nlinks = 0;
    if ((bits & PHX_CXP_LINK1_USED) != 0) {
        links[nlinks++] = 1;
    }
    if ((bits & PHX_CXP_LINK2_USED) != 0) {
        links[nlinks++] = 2;
    }
    if ((bits & PHX_CXP_LINK3_USED) != 0) {
        links[nlinks++] = 3;
    }
    if ((bits & PHX_CXP_LINK4_USED) != 0) {
        links[nlinks++] = 4;
    }
    if (nlinks == 0) {
        fprintf(stream, "%sNo CoaXPress links in use\n", pfx);
    } else if (nlinks == 1) {
        fprintf(stream, "%sCoaXPress link %d in use\n", pfx, links[0]);
    } else {
        fprintf(stream, "%sCoaXPress links ", pfx);
        for (int i = 0; i < nlinks; ++i) {
            fprintf(stream, "%d%s", links[i],
                    (i < nlinks - 2 ? ", " :
                     (i == nlinks - 2 ? " and " : " are in use\n")));
        }
    }
    PRT(PHX_CXP_LINK1_MASTER, "CoaXPress link 1 is the master link");
    PRT(PHX_CXP_LINK2_MASTER, "CoaXPress link 2 is the master link");
    PRT(PHX_CXP_LINK3_MASTER, "CoaXPress link 3 is the master link");
    PRT(PHX_CXP_LINK4_MASTER, "CoaXPress link 4 is the master link");

    if (bits != 0) {
        phx_value_t val;
        if (phx_get(cam, PHX_CXP_BITRATE, &val) != 0) {
            return -1;
        }
        switch ((int)val) {
            CASE(PHX_CXP_BITRATE_UNKNOWN, "No CoaXPress camera is connected, "
                 "or a camera has not completed discovery");
            CASE(PHX_CXP_BITRATE_CXP1, "Bitrate is 1250 Mbps");
            CASE(PHX_CXP_BITRATE_CXP2, "Bitrate is 2500 Mbps");
            CASE(PHX_CXP_BITRATE_CXP3, "Bitrate is 3125 Mbps");
            CASE(PHX_CXP_BITRATE_CXP5, "Bitrate is 5000 Mbps");
            CASE(PHX_CXP_BITRATE_CXP6, "Bitrate is 6250 Mbps");
        }
        if (phx_get(cam, PHX_CXP_DISCOVERY, &val) != 0) {
            return -1;
        }
        switch ((int)val) {
            CASE(PHX_CXP_DISCOVERY_UNKNOWN, "No CoaXPress camera is connected, "
                 "or a camera has not completed discovery");
            CASE(PHX_CXP_DISCOVERY_1X,
                 "The camera is using a single CoaXPress link");
            CASE(PHX_CXP_DISCOVERY_2X,
                 "The camera is using two CoaXPress links");
            CASE(PHX_CXP_DISCOVERY_4X,
                 "The camera is using four CoaXPress links");
        }
        if (phx_get(cam, PHX_CXP_BITRATE_MODE, &val) != 0) {
            return -1;
        }
        switch ((int)val) {
            CASE1(PHX_CXP_BITRATE_MODE_AUTO);
            CASE1(PHX_CXP_BITRATE_MODE_CXP1);
            CASE1(PHX_CXP_BITRATE_MODE_CXP2);
            CASE1(PHX_CXP_BITRATE_MODE_CXP3);
            CASE1(PHX_CXP_BITRATE_MODE_CXP5);
            CASE1(PHX_CXP_BITRATE_MODE_CXP6);
        }
        if (phx_get(cam, PHX_CXP_DISCOVERY_MODE, &val) != 0) {
            return -1;
        }
        switch ((int)val) {
            CASE1(PHX_CXP_DISCOVERY_MODE_AUTO);
            CASE1(PHX_CXP_DISCOVERY_MODE_1X);
            CASE1(PHX_CXP_DISCOVERY_MODE_2X);
            CASE1(PHX_CXP_DISCOVERY_MODE_4X);
        }
        if (phx_get(cam, PHX_CXP_POCXP_MODE, &val) != 0) {
            return -1;
        }
        switch ((int)val) {
            CASE1(PHX_CXP_POCXP_MODE_AUTO);
            CASE1(PHX_CXP_POCXP_MODE_OFF);
            CASE1(PHX_CXP_POCXP_MODE_TRIP_RESET);
            CASE1(PHX_CXP_POCXP_MODE_FORCEON);
        }
    }
#undef CASE
#undef CASE1
#undef PRT
    return 0;
}

int
phx_set_coaxpress_connection(phx_camera_t* cam, const phx_connection_t* con)
{
    phx_value_t bitrate, discovery;
    switch (con->speed) {
    case    0: bitrate = PHX_CXP_BITRATE_MODE_AUTO; break;
    case 1250: bitrate = PHX_CXP_BITRATE_MODE_CXP1; break;
    case 2500: bitrate = PHX_CXP_BITRATE_MODE_CXP2; break;
    case 3125: bitrate = PHX_CXP_BITRATE_MODE_CXP3; break;
    case 5000: bitrate = PHX_CXP_BITRATE_MODE_CXP5; break;
    case 6250: bitrate = PHX_CXP_BITRATE_MODE_CXP6; break;
    default:
        tao_push_error(&cam->errs, __func__, TAO_BAD_SPEED);
        return -1;
   }
    switch (con->channels) {
    case 0: discovery = PHX_CXP_DISCOVERY_MODE_AUTO; break;
    case 1: discovery = PHX_CXP_DISCOVERY_MODE_1X;   break;
    case 2: discovery = PHX_CXP_DISCOVERY_MODE_2X;   break;
    case 4: discovery = PHX_CXP_DISCOVERY_MODE_4X;   break;
    default:
        tao_push_error(&cam->errs, __func__, TAO_BAD_CHANNELS);
        return -1;
    }
    if (bitrate != 0) {
        if (phx_set(cam, PHX_CACHE_FLUSH, PHX_DUMMY_PARAM) != 0) {
            return -1;
        }
        if (discovery != 0) {
            if (phx_set(cam, PHX_CXP_DISCOVERY_MODE, discovery) != 0) {
                return -1;
            }
        }
        if (phx_set(cam, PHX_CXP_BITRATE_MODE|PHX_CACHE_FLUSH, bitrate) != 0) {
            return -1;
        }
    }
    return 0;
}

/*
 * The macro `DST_FORMATS` provide all known (destination) pixel formats.
 * Macro `DST_FORMAT(f,t,b)` have to be defined before evaluationg this macro.
 * An example of usage is in `phx_capture_format_bits`.
 */
#define DST_FORMATS                                                \
    DST_FORMAT(PHX_DST_FORMAT_Y8,     Monochrome,  8)              \
    DST_FORMAT(PHX_DST_FORMAT_Y10,    Monochrome, 10)              \
    DST_FORMAT(PHX_DST_FORMAT_Y12,    Monochrome, 12)              \
    DST_FORMAT(PHX_DST_FORMAT_Y12B,   Monochrome, 12) /* FIXME: */ \
    DST_FORMAT(PHX_DST_FORMAT_Y14,    Monochrome, 14)              \
    DST_FORMAT(PHX_DST_FORMAT_Y16,    Monochrome, 16)              \
    DST_FORMAT(PHX_DST_FORMAT_Y32,    Monochrome, 32)              \
    DST_FORMAT(PHX_DST_FORMAT_Y36,    Monochrome, 36)              \
    DST_FORMAT(PHX_DST_FORMAT_2Y12,   Monochrome, 24)              \
    DST_FORMAT(PHX_DST_FORMAT_BAY8,   BayerFormat,  8)             \
    DST_FORMAT(PHX_DST_FORMAT_BAY10,  BayerFormat, 10)             \
    DST_FORMAT(PHX_DST_FORMAT_BAY12,  BayerFormat, 12)             \
    DST_FORMAT(PHX_DST_FORMAT_BAY14,  BayerFormat, 14)             \
    DST_FORMAT(PHX_DST_FORMAT_BAY16,  BayerFormat, 16)             \
    DST_FORMAT(PHX_DST_FORMAT_RGB15,  RGB, 15)                     \
    DST_FORMAT(PHX_DST_FORMAT_RGB16,  RGB, 16)                     \
    DST_FORMAT(PHX_DST_FORMAT_RGB24,  RGB, 24)                     \
    DST_FORMAT(PHX_DST_FORMAT_RGB32,  RGB, 32)                     \
    DST_FORMAT(PHX_DST_FORMAT_RGB36,  RGB, 36)                     \
    DST_FORMAT(PHX_DST_FORMAT_RGB48,  RGB, 48)                     \
    DST_FORMAT(PHX_DST_FORMAT_BGR15,  BGR, 15)                     \
    DST_FORMAT(PHX_DST_FORMAT_BGR16,  BGR, 16)                     \
    DST_FORMAT(PHX_DST_FORMAT_BGR24,  BGR, 24)                     \
    DST_FORMAT(PHX_DST_FORMAT_BGR32,  BGR, 32)                     \
    DST_FORMAT(PHX_DST_FORMAT_BGR36,  BGR, 36)                     \
    DST_FORMAT(PHX_DST_FORMAT_BGR48,  BGR, 48)                     \
    DST_FORMAT(PHX_DST_FORMAT_BGRX32, BGRX, 32)                    \
    DST_FORMAT(PHX_DST_FORMAT_RGBX32, RGBX, 32)                    \
    /* FIXME: PHX_DST_FORMAT_RRGGBB8 => 8 */                       \
    DST_FORMAT(PHX_DST_FORMAT_XBGR32, XBGR, 32)                    \
    DST_FORMAT(PHX_DST_FORMAT_XRGB32, XRGB, 32)                    \
    DST_FORMAT(PHX_DST_FORMAT_YUV422, YUV422, 16)

uint32_t
phx_capture_format_bits(phx_value_t fmt)
{
    switch (fmt) {
#define DST_FORMAT(f,t,b) case f: return b;
        DST_FORMATS
#undef DST_FORMAT
    default: return 0;
    }
}

int
phx_capture_format_type(phx_value_t fmt)
{
    const int Monochrome  = 1;
    const int BayerFormat = 2;
    const int RGB         = 3;
    const int BGR         = 4;
    const int RGBX        = 5;
    const int BGRX        = 6;
    const int XRGB        = 7;
    const int XBGR        = 8;
    const int YUV422      = 9;
    switch (fmt) {
#define DST_FORMAT(f,t,b) case f: return t;
        DST_FORMATS
#undef DST_FORMAT
    default: return 0;
    }
}


#ifdef _PHX_POSIX
static int              peek_character = -1;
static struct termios   initial_settings, new_settings;
#endif
#ifdef _PHX_VXWORKS
volatile int gnKbHitCount;
volatile int gnKbHitCountMax = 100;
#endif

void
phx_keyboard_init(void)
{
#ifdef _PHX_POSIX
    tcgetattr(0, &initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag    &= ~ICANON;
    new_settings.c_lflag    &= ~ECHO;
    new_settings.c_lflag    &= ~ISIG;
    new_settings.c_cc[VMIN]  = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
#elif defined _PHX_VXWORKS
    gnKbHitCount = 0;
#else
    /* Nothing to do */
#endif
}

int
phx_keyboard_hit()
{
#if defined _PHX_POSIX
    char  ch;
    int   nread;

    if (peek_character != -1) {
        return 1;
    }
    new_settings.c_cc[VMIN] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0, &ch, 1);
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_settings);

    if (nread == 1) {
        peek_character = ch;
        return 1;
    }
    return 0;
#elif defined _PHX_VXWORKS
    return (gnKbHitCount++ > gnKbHitCountMax);
#elif defined _MSC_VER
    return _kbhit();
#else
    return kbhit();
#endif
}

void
phx_keyboard_final(void)
{
#ifdef _PHX_POSIX
    tcsetattr(0, TCSANOW, &initial_settings);
#elif defined _PHX_VXWORKS
    /* TODO */
#else
    /* Nothing to do */
#endif
}

int
phx_keyboard_read(void)
{
#ifdef _PHX_POSIX
    char ch;
    int nBytes;

    if (peek_character != -1) {
        ch = peek_character;
        peek_character = -1;
        return ch;
    }
    nBytes = read(0, &ch, 1);
    return (nBytes == 1 ? ch : '\0');
#elif defined _PHX_VXWORKS
    /* TODO */
#elif defined _MSC_VER
    return _getch();
#else
    return getch();
#endif
}
