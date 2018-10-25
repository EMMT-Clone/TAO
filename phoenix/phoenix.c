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
#include <termios.h>
#endif

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

#define lock_mutex(cam) tao_lock_mutex(&(cam)->errs, &(cam)->mutex)

#define unlock_mutex(cam) tao_unlock_mutex(&(cam)->errs, &(cam)->mutex)

#define try_lock_mutex(cam) tao_try_lock_mutex(&(cam)->errs, &(cam)->mutex)

#define signal_condition(cam)                       \
    tao_signal_condition(&(cam)->errs, &(cam)->cond)

/*---------------------------------------------------------------------------*/
/* ERROR MANAGEMENT */

/* Callback for TAO error management system. */
static void
get_error_details(int code, const char** reason, const char** info)
{
    if (reason != NULL) {
        *reason = "ActiveSilicon Phoenix library reports that an error occured";
    }
    if (info != NULL) {
        *info = phx_status_string(code);
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
static int error_handler_level = 2;

void
phx_set_error_handler_verbosity(int level)
{
  if (pthread_mutex_lock(&error_handler_mutex) == 0) {
    error_handler_level = level;
    (void)pthread_mutex_unlock(&error_handler_mutex);
  }
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
    if (level >= 2 && reason != NULL && reason[0] != '\0') {
        fprintf(stderr, "Function %s failed with code 0x%08x.\n%s\n",
                funcname, errcode, reason);
    } else if (level >= 1) {
        fprintf(stderr, "Function %s failed with code 0x%08x.\n",
                funcname, errcode);
    }
  }
}

const char*
phx_status_string(phx_status_t status)
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
        while (bufs[i].pvAddress != NULL) {
            ++i;
        }
        while (--i >= 0) {
            free(bufs[i].pvAddress);
        }
        cam->bufs = NULL;
        free(bufs);
    }
}

static int
allocate_virtual_buffers(phx_camera_t* cam, int nbufs, long* bufstride)
{
    /* Check arguments. */
    if (nbufs < 1) {
        phx_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
        return -1;
    }

    /* Get buffer size. */
    phx_value_t bufwidth, bufheight;
    if (phx_get(cam, PHX_BUF_DST_XLENGTH, &bufwidth) != 0 ||
        phx_get(cam, PHX_BUF_DST_YLENGTH, &bufheight) != 0) {
        return -1;
    }
    size_t bufsize = (size_t)bufwidth*(size_t)bufheight;
    if (bufstride != NULL) {
        *bufstride = bufwidth;
    }

    if (cam->bufs == NULL || cam->nbufs != nbufs || cam->bufsize != bufsize) {
        /* Allocate new image buffers */
        free_virtual_buffers(cam);
        cam->bufs = NEW(&cam->errs, phx_imgbuf_t, nbufs + 1);
        if (cam->bufs == NULL) {
            return -1;
        }
        for (int i = 0; i < nbufs; ++i) {
            cam->bufs[i].pvAddress = (uint8_t*)tao_malloc(&cam->errs, bufsize);
            if (cam->bufs[i].pvAddress == NULL) {
                free_virtual_buffers(cam);
                return -1;
            }
            cam->bufs[i].pvContext = (void*)(ptrdiff_t)i;
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
    phx_value_t value = PHX_DST_PTR_USER_VIRT;
    if (phx_set_parameter(cam, (PHX_DST_PTR_TYPE|PHX_CACHE_FLUSH|
                                PHX_FORCE_REWRITE), &value) != 0) {
        free_virtual_buffers(cam); /* FIXME: we can keep the virtual buffers? */
        return -1;
    }

    return 0;
}

/*
 * Callback for acquisition.
 */
static void
acquisition_callback(phx_handle_t handle, uint32_t events, void* data)
{
    phx_camera_t* cam = (phx_camera_t*)data;
    phx_imgbuf_t imgbuf;
    struct timespec ts;

    /* Lock the context and make minimal sanity check. */
    if (lock_mutex(cam) != 0) {
        return;
    }
    if (cam->handle != handle) {
        tao_push_error(&cam->errs, __func__, TAO_ASSERTION_FAILED);
        goto unlock;
    }

    /* Account for events. */
    if ((PHX_INTRPT_BUFFER_READY & events) != 0) {
        /* A new frame is available. */
        /* Take its arrival time stamp.  FIXME: time stamp must be attached to the buffer */
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
            cam->last_buffer = imgbuf.pvAddress;
            cam->last_index = (long)imgbuf.pvContext;
            cam->last_time_s = ts.tv_sec;
            cam->last_time_ns = ts.tv_nsec;
            ++cam->frames;
            ++cam->pending;
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
    if ((cam->events & events) != 0) {
        /* Signal condition for waiting thread. */
        signal_condition(cam);
    }

    /* Unlock the context. */
 unlock:
    unlock_mutex(cam);
}

int
phx_start(phx_camera_t* cam, int nbufs)
{

  /* Minimal checks. */
  if (cam == NULL) {
    return -1;
  }
  if (cam->state < 1) {
      // FIXME: change error code
      tao_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
      return -1;
  }
  if (cam->state > 1) {
      tao_push_error(&cam->errs, __func__, TAO_ACQUISITION_RUNNING);
      return -1;
  }

  /* Allocate virtual buffers. */
  if (allocate_virtual_buffers(cam, nbufs, NULL) != 0) {
      return -1;
  }

  /* Enable interrupts for handled events. */
  phx_value_t events = (PHX_INTRPT_BUFFER_READY |
                        PHX_INTRPT_FIFO_OVERFLOW |
                        PHX_INTRPT_SYNC_LOST);
  if (phx_set(cam, PHX_INTRPT_CLR, ~(phx_value_t)0) != 0 ||
      phx_set(cam, PHX_INTRPT_SET, events|PHX_INTRPT_GLOBAL_ENABLE) != 0) {
      return -1;
  }

  /* Setup callback context. */
  if (phx_set_parameter(cam, PHX_EVENT_CONTEXT, (void*)cam) != 0) {
      return -1;
  }

  /* Start acquisition with given callback. */
  if (phx_read_stream(cam, PHX_START, acquisition_callback) != 0) {
      return -1;
  }

  /* Send specific start command. */
  if (cam->start != NULL && cam->start(cam) != 0) {
      phx_read_stream(cam, PHX_ABORT, acquisition_callback);
      phx_read_stream(cam, PHX_UNLOCK, NULL);
      return -1;
  }

  /* Update state and return. */
  cam->state = 2;
  return 0;
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
phx_wait_image(phx_camera_t* cam, double secs, int drop)
{
    struct timespec ts;
    int forever, code, status = 0;

    /* Check state. */
    if (cam->state != 2) {
        if (cam->state == 0 || cam->state == 1) {
            tao_push_error(&cam->errs, __func__, TAO_NO_ACQUISITION);
        } else {
            tao_push_error(&cam->errs, __func__, TAO_CORRUPTED);
        }
        return -1;
    }

    /* Compute absolute time for time-out. */
    if (isnan(secs) || secs < 0) {
        tao_push_error(&cam->errs, __func__, TAO_BAD_ARGUMENT);
        return -1;
    }
    if (secs > TAO_YEAR) {
        forever = TRUE;
    } else {
        if (tao_get_absolute_timeout(&cam->errs, &ts, secs) != 0) {
            return -1;
        }
        forever = FALSE;
    }

    /*
     * Lock mutex and wait for next image, taking care to not throw anything
     * while the mutex is locked.
     */
    if (lock_mutex(cam) != 0) {
        return -1;
    }

    /*
     * While there are no pending frames, wait for the condition to be
     * signaled or an error to occur.  This is done in a `while` loop to
     * cope with spurious signaled conditions.
     */
    while (cam->pending == 0) {
        if (forever) {
            code = pthread_cond_wait(&cam->cond, &cam->mutex);
            if (code != 0) {
                tao_push_error(&cam->errs, "pthread_cond_wait", code);
                status = -1;
                goto unlock;
            }
        } else {
            code = pthread_cond_timedwait(&cam->cond, &cam->mutex, &ts);
            if (code != 0) {
                if (code == ETIMEDOUT) {
                    status = 0;
                } else {
                    tao_push_error(&cam->errs, "pthread_cond_timedwait", code);
                    status = -1;
                }
                goto unlock;
            }
        }
    }

    if (drop) {
        /* Get rid of unprocessed pending image buffers. */
        while (cam->pending > 1) {
            if (phx_read_stream(cam, PHX_BUFFER_RELEASE, NULL) != 0) {
                status = -1;
                goto unlock;
            }
            --cam->pending;
            ++cam->overflows;
        }
    }

    if (cam->pending >= 1) {
        /* If no errors occured so far and at least one image buffer is
         * unprocessed, manage to return the index of this buffer.
         */
        --cam->pending;
        cam->captured_index = cam->last_index - cam->pending;
        while (cam->captured_index < 0) {
            /* FIXME: use modulo arithmetic */
            cam->captured_index += cam->nbufs;
        }
    }

    /* Unlock the mutex (whatever the errors so far). */
 unlock:
    if (unlock_mutex(cam) != 0) {
        return -1;
    }
    return status;
}

/*--------------------------------------------------------------------------*/
/* WRAPPERS FOR PHOENIX ROUTINES */

static int
check_coaxpress(phx_camera_t* cam)
{
  phx_value_t info;
  uint32_t magic, width, height;

  if (phx_get_parameter(cam, PHX_CXP_INFO, &info) != 0) {
      return -1;
  }
  cam->coaxpress = FALSE;
  if ((info & PHX_CXP_CAMERA_DISCOVERED) != 0) {
      /* We have a CoaXPress camera. */
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
      cam->coaxpress = TRUE;

      /* Get sensor width and height (full width and full height must be
         correctly set later). */
      if (cxp_get(cam, WIDTH_ADDRESS, &width) != 0 ||
          cxp_get(cam, HEIGHT_ADDRESS, &height) != 0) {
          return -1;
      }
      cam->fullwidth  = cam->width  = width;
      cam->fullheight = cam->height = height;

    /* Get device vendor name and device model name. */
    if (cxp_get(cam, DEVICE_VENDOR_NAME, cam->vendor) != 0 ||
        cxp_get(cam, DEVICE_MODEL_NAME, cam->model) != 0) {
      return -1;
    }
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
    cam->swap = FALSE;
    cam->coaxpress = FALSE;
    cam->start = NULL;
    cam->stop = NULL;
#if 0
    cam->get_config = no_config;
    cam->set_config = no_config;
#endif

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
            PHX_StreamRead(cam->handle, PHX_ABORT, NULL);
            PHX_StreamRead(cam->handle, PHX_UNLOCK, NULL);
            if (cam->stop != NULL) {
                cam->stop(cam);
            }
            cam->state = 1;
        }
        if (cam->state >= 1) {
            /* Close the Phoenix board. */
            PHX_Close(&cam->handle);
            cam->state = 0;
        }
        if (cam->state >= 0) {
            /* Destroy the Phoenix handle and release memory. */
            PHX_Destroy(&cam->handle);
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

/* These wrappers use TAO error management system. */

int
phx_read_stream(phx_camera_t* cam, phx_acquisition_t command, void* addr)
{
   phx_status_t status = PHX_StreamRead(cam->handle, command, addr);
   if (status != PHX_OK) {
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
   } else {
       return 0;
   }
}

int
cxp_write(phx_camera_t* cam, uint32_t addr, uint8_t* data, uint32_t* size)
{
    phx_status_t status = PHX_ControlWrite(cam->handle, PHX_REGISTER_DEVICE,
                                           &addr, data, size, cam->timeout);
    if (status != PHX_OK) {
        phx_push_error(&cam->errs, "PHX_ControlWrite", status);
       return -1;
   } else {
       return 0;
   }
}

int
cxp_reset(phx_camera_t* cam, uint32_t addr)
{
    phx_status_t status = PHX_ControlReset(cam->handle, PHX_REGISTER_DEVICE,
                                           &addr, cam->timeout);
    if (status != PHX_OK) {
        phx_push_error(&cam->errs, "PHX_ControlReset", status);
       return -1;
   } else {
       return 0;
   }
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
