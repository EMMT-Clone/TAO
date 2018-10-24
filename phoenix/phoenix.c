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
 * Copyright (C) 2016, Éric Thiébaut & Jonathan Léger.
 * Copyright (C) 2017-2018, Éric Thiébaut.
 */

#include "phoenix.h"

#ifdef _PHX_POSIX
#include <termios.h>
#endif

#define MIN(a,b) ((a) <= (b) ? (a) : (b))

#define MAX(a,b) ((a) >= (b) ? (a) : (b))

#define NEW(errs, type) ((type*)tao_calloc(errs, 1, sizeof(type)))

/*---------------------------------------------------------------------------*/
/* ERROR MANAGEMENT */

/* Callback for TAO error management system. */
static void
get_error_details(int code, const char** reason, const char** info)
{
    static char buffer[20]; /* FIXME: not thread safe */
    *reason = "ActiveSilicon Phoenix library reports that an error occured";
    *info = phx_status_string(buffer, code);
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

/* buffer must have at least 20 bytes (enough to print any value of a 64-bit
   signed integer in decimal form). */
const char*
phx_status_string(char* buffer, phx_status_t status)
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
        sprintf(buffer, "%d", status);
        return buffer;
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

    /* Check assumptions. */
    assert(sizeof(ui8)  == 1);
    assert(sizeof(ui16) == 2);
    assert(sizeof(ui32) == 4);
    assert(sizeof(ui64) == 8);
    assert(sizeof(float32_t) == 4);
    assert(sizeof(float64_t) == 8);

    /* Allocate memory to store the camera instance. */
    cam = NEW(errs, phx_camera_t);
    if (cam == NULL) {
        return NULL;
    }
    cam->timeout = 500;
    cam->state = -3;
    cam->swap = FALSE;
    cam->coaxpress = FALSE;
#if 0
    cam->start = no_op;
    cam->stop = no_op;
    cam->get_config = no_config;
    cam->set_config = no_config;
#endif
    pthread_mutex_init(&cam->mutex, NULL); // FIXME: check
    cam->state = -2;
    pthread_cond_init(&cam->cond, NULL); // FIXME: check
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
    //if (cam->coaxpress) {
    //    /* Apply specific camera initialization. */
    //    if (phx_check_MikrotronMC408x(cam)) {
    //        status = phx_initialize_MikrotronMC408x(cam);
    //        if (status != PHX_OK) {
    //            goto done;
    //        }
    //    } else {
    //        fprintf(stderr, "WARNING: Unknown CoaXPress camera, "
    //                "initialization may be incomplete.\n");
    //    }
    //}

    return cam;

 error:
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
            //FIXME: cam->stop(cam);
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

#define DEF_FUNC(SUFFIX, TYPE)                          \
    int                                                 \
    phx_set_##SUFFIX##_parameter(phx_camera_t* cam,     \
                                 phx_param_t   param,   \
                                 TYPE          value)   \
    {                                                   \
        return phx_set_parameter(cam, param, &value);   \
    }
DEF_FUNC(enum,   int)
DEF_FUNC(uint32, uint32_t)
DEF_FUNC(uint64, uint64_t)
#undef DEF_FUNC

#define DEF_FUNC(SUFFIX, TYPE)                          \
    int                                                 \
    phx_get_##SUFFIX##_parameter(phx_camera_t* cam,     \
                                 phx_param_t   param,   \
                                 TYPE*         value)   \
    {                                                   \
        return phx_get_parameter(cam, param, value);    \
    }
DEF_FUNC(enum,   int)
DEF_FUNC(uint32, uint32_t)
DEF_FUNC(uint64, uint64_t)
#undef DEF_FUNC

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
