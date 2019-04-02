/*
 * andor-server.c --
 *
 * Image server for Andor cameras.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <xpa.h>
#include <andor.h>
#include <tao.h>
#include <tao-private.h>

/* These are the tasks that can be scheduled.  The policy is as follows: the
   master must lock the resources to schedule a new task, EXIT is irreversible,
   after a START, any of STOP, ABORT and EXIT are possible. */
typedef enum {
    COMMAND_NONE,  /* Nothing to do */
    COMMAND_START, /* Request to start */
    COMMAND_STOP,  /* Request to stop */
    COMMAND_ABORT, /* Request to abort */
    COMMAND_EXIT,  /* Request to exit */
} command_t;

static const char* command_name(command_t cmd)
{
    switch (cmd) {
    case COMMAND_NONE:  return "none";
    case COMMAND_START: return "start";
    case COMMAND_STOP:  return "stop";
    case COMMAND_ABORT: return "abort";
    case COMMAND_EXIT:  return "exit";
    default:            return "unknown";
    }
}

/* These are the tasks that are currently been executed.  The worker thread,
   accounts for a new scheduled task when sleeping and a notifification is
   signaled or after every acquired frame (or timeout while waiting for a new
   frame).  When a new scheduled task is taken into account by the worker
   thread, it changes the value of its `state` parameter and when the
   task is completed,it changes the value of its `command` parameter. */
typedef enum {
    STATE_SLEEPING,  /* Sleeping */
    STATE_STARTING,  /* Starting acquisition */
    STATE_ACQUIRING, /* Acquisition running */
    STATE_STOPPING,  /* Stopping acquisition */
    STATE_ABORTING,  /* Aborting acquisition */
    STATE_DONE,      /* Thread no longer available */
} state_t;

static const char* state_name(state_t state)
{
    switch (state) {
    case STATE_SLEEPING:  return "sleeping";
    case STATE_STARTING:  return "starting";
    case STATE_ACQUIRING: return "acquiring";
    case STATE_STOPPING:  return "stopping";
    case STATE_ABORTING:  return "aborting";
    case STATE_DONE:      return "done";
    default:              return "unknown";
    }
}

/*
 * Life cycle of the processing thread:
 *
 *  +----> NONE    :::> SLEEPING
 *  |       |
 *  |       V
 *  |     START    :::> STARTING => ACQUIRING
 *  |       |
 *  |       V
 *  +-- STOP/ABORT :::> STOPPING/ABORTING => SLEEPING
 *
 *  NONE/START/STOP/ABORT
 *          |
 *          V           SLEEPING => EXITING
 *         EXIT    :::> STARTING => ABORTING => EXITING
 *                      ACQUIRING => ABORTING => EXITING
 *
 * Legend: ---> allowed "moves" for scheduling tasks
 *         :::> consequence of a scheduled task
 *         ===> temporal evolution of the processing thread
 *
 * Note that the only difference between "stop" and "abort" is that "abort"
 * stops acquisition as soon as possible while "stop" process the current
 * image, if any, before stopping acquisition.
 */

/* Callback for processing acquisition buffers. */
typedef void processor(void* data, struct timespec* ts,
                       const void* buf, long siz);

/* Data shared between the main thread and the image processing thread is
   stored in the following structure.  This is not strictly necessary (static
   variables could have been used instead) but this is cleaner and helps to
   clarify the code. */
typedef struct private_data private_data_t;
struct private_data {
    pthread_mutex_t mutex; /* Lock to protect this structure */
    pthread_cond_t notify; /* Condition variable for notifications */
    pthread_t      worker; /* Acquisition and processing thread */
    andor_camera_t*   cam; /* The true camera */
    command_t     command; /* Command to execute */
    state_t         state; /* Current state of the thread */
    processor*       proc; /* Callback processing acquisition buffers */
    void*            data; /* Associated data */
    double        timeout; /* Timeout for waiting next frame (seconds) */
    int             nbufs; /* Number of acquisition buffers */
    long         timeouts; /* Number of timouts so far */
    long           frames; /* Number of frames so far */
};

static private_data_t*
create_private_data(andor_camera_t* cam)
{
    private_data_t* priv;

    priv = (private_data_t*)tao_calloc(&cam->errs, 1, sizeof(private_data_t));
    if (priv == NULL) {
        return NULL;
    }
    if (tao_initialize_mutex(&cam->errs, &priv->mutex, false) != 0) {
        free((void*)priv);
        return NULL;
    }
    if (tao_initialize_condition(&cam->errs, &priv->notify) != 0) {
        (void)tao_destroy_mutex(&cam->errs, &priv->mutex, true);
        free((void*)priv);
        return NULL;
    }
    priv->cam = cam;
    priv->command = COMMAND_NONE;
    priv->state = STATE_SLEEPING;
    priv->proc = NULL;
    priv->data = NULL;
    priv->timeout = 0.1;
    priv->nbufs = 4;
    priv->timeouts = 0;
    priv->frames = 0;
    return priv;
}

static void
destroy_private_data(private_data_t* shared)
{
    if (shared != NULL) {
        (void)tao_destroy_mutex(NULL, &shared->mutex, true);
        (void)tao_destroy_condition(NULL, &shared->notify);
        free((void*)shared);
    }
}

/* Global variables. */
static const char* progname = "andor-server";
static bool debug = false;
static bool quit = false;
static tao_buffer_t srvbuf; /* dynamic i/o buffer for answering requests */
static tao_camera_t* srvcam = NULL; /* server camera for communicating
                                     * information with clients */
static private_data_t* priv = NULL;

/* Report TAO errors to XPA client. */
static void report_errors(XPA xpa, tao_error_t** errs);

/* Like XPAError but use the dynamic i/o buffer to format and report an error
   to XPA. */
static void xpa_error(XPA xpa, const char* format, ...);

/* The following macros or private functions are to simplify the code, they all
   assume that errors are unrecoverable. */
#define get_monotonic_time(ts) tao_get_monotonic_time(NULL, ts)
#define lock_mutex(priv) tao_lock_mutex(NULL, &priv->mutex)
#define unlock_mutex(priv) tao_unlock_mutex(NULL, &priv->mutex)
#define trylock_mutex(priv) tao_trylock_mutex(NULL, &priv->mutex)
#define signal_condition(priv) tao_signal_condition(NULL, &priv->cond)
#define wait_condition(priv) tao_wait_condition(NULL, &priv->notify,    \
                                                &priv->mutex)
#define fatal_error(priv)                               \
    do {                                                \
        andor_report_errors(priv->cam);                 \
        if (priv->cam->state == 2) {                    \
            (void)andor_stop_acquisition(priv->cam);    \
        }                                               \
        exit(EXIT_FAILURE);                             \
    } while (false)
#define die_if(expr, priv)                      \
    do {                                        \
        if (expr) {                             \
            fatal_error(priv);                  \
        }                                       \
    } while (false)
#define start_acquisition(priv) \
    die_if(andor_start_acquisition(priv->cam, priv->nbufs) != 0, priv)
#define queue_buffer(priv, buf, siz) \
    die_if(andor_queue_buffer(priv->cam, buf, siz) != 0, priv)
#define stop_acquisition(priv) \
    die_if(andor_stop_acquisition(priv->cam) != 0, priv)

static void process_frame(void* unused, struct timespec* ts,
                          const void* buf, long siz)
{
    tao_shared_array_t* arr;
    andor_encoding_t arrenc, bufenc;
    long width, height, stride;

    /* It is guaranteed that the configuration will not change while
       processing the buffer, so we do not have to lock the private data
       while processing. */
    width = priv->cam->config.width;
    height = priv->cam->config.height;
    stride = priv->cam->stride;
    bufenc = priv->cam->config.pixelencoding;
    if (siz > height*stride) {
        tao_push_error(NULL, __func__, TAO_ASSERTION_FAILED);
    }

    tao_lock_shared_camera(NULL, srvcam->shared);
    arr = tao_fetch_next_frame(NULL, srvcam);
    tao_unlock_shared_camera(NULL, srvcam->shared);

    switch (TAO_GET_SHARED_ARRAY_ELTYPE(arr)) {
    case TAO_UINT8:
    case TAO_INT8:
        arrenc = ANDOR_ENCODING_MONO8;
        break;
    case TAO_UINT16:
    case TAO_INT16:
        arrenc = ANDOR_ENCODING_MONO16;
        break;
    case TAO_INT32:
    case TAO_UINT32:
        arrenc = ANDOR_ENCODING_MONO32;
        break;
    case TAO_FLOAT:
        arrenc = ANDOR_ENCODING_FLOAT;
        break;
    case TAO_DOUBLE:
        arrenc = ANDOR_ENCODING_DOUBLE;
        break;
    default:
        arrenc = ANDOR_ENCODING_UNKNOWN;
    }

    if (arrenc != ANDOR_ENCODING_UNKNOWN &&
        TAO_GET_SHARED_ARRAY_NDIMS(arr) >= 2 &&
        TAO_GET_SHARED_ARRAY_DIM(arr, 1) == width &&
        TAO_GET_SHARED_ARRAY_DIM(arr, 2) == height) {
        andor_convert_buffer(TAO_GET_SHARED_ARRAY_DATA(arr), arrenc,
                             buf, bufenc, width, height, stride);
        if (ts != NULL) {
            arr->ts_sec  = ts->tv_sec;
            arr->ts_nsec = ts->tv_nsec;
        }
        tao_lock_shared_camera(NULL, srvcam->shared);
        (void)tao_publish_next_frame(NULL, srvcam, arr);
        tao_unlock_shared_camera(NULL, srvcam->shared);
    }
}

static void broadcast_state(int state)
{
    tao_shared_camera_t* cam = srvcam->shared;
    tao_lock_shared_camera(NULL, cam);
    cam->state = state;
    tao_unlock_shared_camera(NULL, cam);
}

/* This is the function executed by the image processing thread. */
static void* run_acquisition(void* arg)
{
    lock_mutex(priv);
    priv->state = STATE_SLEEPING;
    broadcast_state(priv->state);
    while (priv->command != COMMAND_EXIT) {

        /* Wait for a new task to be scheduled taking care of spurious
           wake-ups. */
        while (priv->command == COMMAND_NONE) {
            wait_condition(priv);
            if (debug) {
                fprintf(stderr, "cmd: %s\n", command_name(priv->command));
            }
        }

        if (priv->command == COMMAND_START) {
            /* Start acquisition.  Since starting can take some time, it is
               checked whether a new task has been scheduled in the
               meantime. */
            priv->state = STATE_STARTING;
            broadcast_state(priv->state);
            if (priv->cam->state == 1) {
                unlock_mutex(priv);
                start_acquisition(priv);
                lock_mutex(priv);
            }
            if (priv->command == COMMAND_START) {
                /* No new command has been scheduled while starting.  Indicate
                   that start was done and that we are now acquiring.  Then
                   execute acquisition loop.  */
                priv->command = COMMAND_NONE;
                priv->state = STATE_ACQUIRING;
                broadcast_state(priv->state);
                while (priv->command == COMMAND_NONE) {
                    struct timespec ts;
                    void* buf;
                    long siz;
                    int status;

                    /* Wait for new acquisition buffer and get time of
                       arrival. */
                    unlock_mutex(priv);
                    status = andor_wait_buffer(priv->cam, &buf, &siz,
                                               priv->timeout);
                    if (status == -1) {
                        /* An error occured. */
                        fatal_error(priv);
                    }
                    get_monotonic_time(&ts);
                    lock_mutex(priv);

                    /* Check whether a new command was scheduled while waiting,
                       then perform any relevant action. */
                    if (priv->command == COMMAND_START) {
                        /* Just ignore new start commands. */
                        priv->command = COMMAND_NONE;
                    }
                    if (priv->command == COMMAND_NONE ||
                        priv->command == COMMAND_STOP) {
                        /* Deal with the last acquired frame. */
                        if (status == 0) {
                            /* A timeout occured. */
                            ++priv->timeouts;
                        } else {
                            /* A new acquisition buffer is available.
                               Apply processing if any, then re-queue the
                               buffer.  It is guaranteed that the
                               configuration will not change while
                               processing the buffer. */
                            processor* proc = priv->proc;
                            void* data = priv->data;
                            if (proc != NULL) {
                                /* It is guaranteed that the configuration
                                   will not change while processing the
                                   buffer, so we can unlock the private
                                   data while processing. */
                                unlock_mutex(priv);
                                proc(data, &ts, buf, siz);
                                lock_mutex(priv);
                            }
                            queue_buffer(priv, buf, siz);
                        }
                    }
                } /* end of acquisition loop */
            }
        } /* COMMAND_START */

        /* Deal with other commands.  This group of tests must be independent
           from the previous (not and `else if` statement) because a new
           command may have been scheduled while starting or acquiring. */
        if (priv->command == COMMAND_STOP || priv->command == COMMAND_ABORT) {
            priv->state = (priv->command == COMMAND_STOP ?
                           STATE_STOPPING : STATE_ABORTING);
            broadcast_state(priv->state);
            if (priv->cam->state == 2) {
                unlock_mutex(priv);
                stop_acquisition(priv);
                lock_mutex(priv);
            }
            if (priv->command == COMMAND_STOP ||
                priv->command == COMMAND_ABORT) {
                /* No new command has been scheduled while stopping.  Indicate
                   that acquiring was stopped and that we are now sleeping. */
                priv->command = COMMAND_NONE;
                priv->state = STATE_SLEEPING;
                broadcast_state(priv->state);
            }
        } /* COMMAND_STOP or COMMAND_ABORT */

    }
    priv->state = STATE_DONE;
    unlock_mutex(priv);
    broadcast_state(priv->state);
    return NULL;
}

static void xpa_error(XPA xpa, const char* format, ...)
{
    void* buf;
    va_list args;
    tao_clear_buffer(&srvbuf);
    va_start(args, format);
    (void)tao_vprint_to_buffer(NULL, &srvbuf, format, args);
    va_end(args);
    (void)tao_get_buffer_contents(&srvbuf, &buf);
    XPAError(xpa, buf);
}

static void report_errors(XPA xpa, tao_error_t** errs)
{
    const char* func;
    const char* reason;
    const char* info;
    tao_error_getter_t* proc;
    int code;
    long nerrs = 0;
    char* buf;
    char infobuf[20];

    /* Write error message. */
    tao_clear_buffer(&srvbuf);
    while (tao_pop_error(errs, &func, &code, &proc)) {
        /*
         * Append next error message to the i/o buffer.  A fatal error results
         * in case of failure here.
         */
        tao_retrieve_error_details(code, &reason, &info, proc, infobuf);
        if (debug) {
            fprintf(stderr, "%s %s in function `%s` [%s]\n",
                    (nerrs > 0 ? "       " : "{ERROR}"), reason, func, info);
        }
        tao_print_to_buffer(NULL, &srvbuf, "%s%s in function `%s` [%s]\n",
                            (nerrs > 0 ? "; " : ""), reason, func, info);
        ++nerrs;
    }
    if (nerrs > 0) {
        (void)tao_get_buffer_contents(&srvbuf, (void**)&buf);
    } else {
        /* There were no errors! */
        buf = "Failure assumed but no error information!";
    }
    XPAError(xpa, buf);
}

static int send_command(XPA xpa, command_t cmd, int nbufs)
{
    int status = 0;
    switch(cmd) {
    case COMMAND_NONE:
        /* Nothing to do. */
        break;
    case COMMAND_START:
    case COMMAND_STOP:
    case COMMAND_ABORT:
    case COMMAND_EXIT:
        lock_mutex(priv);
        if (priv->command == COMMAND_EXIT && cmd != COMMAND_EXIT) {
            XPAError(xpa, "Camera has been closed");
            status = -1;
        } else if (cmd == COMMAND_START && priv->nbufs != nbufs) {
            if (priv->state != STATE_SLEEPING) {
                XPAError(xpa, "Cannot change the number of "
                         "acquisition buffers now");
                status = -1;
            } else {
                priv->nbufs = nbufs;
            }
        }
        if (status == 0) {
            priv->command = cmd;
            tao_signal_condition(NULL, &priv->notify);
        }
        unlock_mutex(priv);
        break;
    default:
        XPAError(xpa, "Invalid command");
        status = -1;
    }
    return status;
}

//static void print_to_buffer(const char* format, ...)
//{
//    va_list args;
//    va_start(args, format);
//    status = tao_vprint_to_buffer(NULL, &srvbuf, format, args);
//    va_end(args);
//}
//
//static void put_string_to_buffer(const char* str)
//{
//    (void)tao_put_string_to_buffer(NULL, &srvbuf, str, -1);
//}

/*---------------------------------------------------------------------------*/
/* GET CALLBACK */

typedef struct get_command get_command_t;
struct get_command {
    const char* name;
    void (*get)(void);
};

#define GETTER(type, format, name, expr)                        \
    static void _get_##name()                                   \
    {                                                           \
        type _val;                                              \
        lock_mutex(priv);                                       \
        _val = (expr);                                          \
        unlock_mutex(priv);                                     \
        (void)tao_print_to_buffer(NULL, &srvbuf, format, _val); \
    }
GETTER(long,   "%ld", sensorwidth,  priv->cam->sensorwidth)
GETTER(long,   "%ld", sensorheight, priv->cam->sensorheight)
GETTER(long,   "%ld", xbin,         priv->cam->config.xbin)
GETTER(long,   "%ld", ybin,         priv->cam->config.ybin)
GETTER(long,   "%ld", xoff,         priv->cam->config.xoff)
GETTER(long,   "%ld", yoff,         priv->cam->config.yoff)
GETTER(long,   "%ld", width,        priv->cam->config.width)
GETTER(long,   "%ld", height,       priv->cam->config.height)
GETTER(double, "%g",  exposuretime, priv->cam->config.exposuretime)
GETTER(double, "%g",  framerate,    priv->cam->config.framerate)

static void _get_debug()
{
    (void)tao_put_string_to_buffer(NULL, &srvbuf, (debug ? "on" : "off"), -1);
}

static void _get_ping()
{
    char smallbuf[32];
    struct timespec ts;
    (void)tao_get_monotonic_time(NULL, &ts);
    tao_sprintf_time(smallbuf, &ts);
    (void)tao_put_string_to_buffer(NULL, &srvbuf, smallbuf, -1);
}

static void _get_roi()
{
    long xoff, yoff, width, height;
    lock_mutex(priv);
    xoff = priv->cam->config.xoff;
    yoff = priv->cam->config.yoff;
    width = priv->cam->config.width;
    height = priv->cam->config.height;
    unlock_mutex(priv);
    (void)tao_print_to_buffer(NULL, &srvbuf, "%ld %ld %ld %ld",
                              xoff, yoff, width, height);
}

static void _get_shmid()
{
    int id = tao_get_shared_camera_ident(srvcam->shared);
    (void)tao_print_to_buffer(NULL, &srvbuf, "%d", id);
}

static void _get_state()
{
    state_t state;
    lock_mutex(priv);
    state = priv->state;
    unlock_mutex(priv);
    (void)tao_put_string_to_buffer(NULL, &srvbuf, state_name(state), -1);
}

static get_command_t getcmds[] = {
#define GETCMD(attr) {#attr, _get_##attr}
     GETCMD(debug),
     GETCMD(exposuretime),
     GETCMD(framerate),
     GETCMD(height),
     GETCMD(ping),
     GETCMD(roi),
     GETCMD(sensorheight),
     GETCMD(sensorwidth),
     GETCMD(shmid),
     GETCMD(state),
     GETCMD(width),
     GETCMD(xbin),
     GETCMD(xoff),
     GETCMD(ybin),
     GETCMD(yoff),
#undef GETCMD
     { NULL, NULL }
};

/* Callback to answer an XPAGet request. */
static int send_callback(void* send_data, void* call_data,
                         char* command, char** bufptr, size_t* lenptr)
{
    /* Automatic variables. */
    XPA xpa = (XPA)call_data;
    tao_error_t* errs = TAO_NO_ERRORS;
    int argc, status = 0;
    long len;
    void* ptr;
    int c;
    const char** argv = NULL;

    /* Split command in a list of arguments. */
    if (debug) {
        fprintf(stderr, "send: \"%s\"\n", command);
    }
    argc = tao_split_command(&errs, &argv, command, -1);
    if (argc < 0) {
        report_errors(xpa, &errs);
        goto failure;
    }
    if (debug) {
        for (int i = 0; i < argc; ++i) {
            fprintf(stderr, "send: [%d] >>%s<<\n", i, argv[i]);
        }
    }
    if (argc == 0) {
        /* No command. */
        goto success;
    }
    if (argc > 1) {
        /* Too many arguments. */
        xpa_error(xpa, "Usage: xpaget %s", argv[0]);
        goto failure;
    }

    /* Parse command starting with an empty answer. */
    tao_clear_buffer(&srvbuf);
    c = argv[0][0];
    for (int i = 0; getcmds[i].name != NULL; ++i) {
        if (getcmds[i].name[0] == c && strcmp(argv[0], getcmds[i].name) == 0) {
            getcmds[i].get();
            goto success;
        }
    }
    xpa_error(xpa, "Unknown parameter for xpaget command");
    goto failure;

 success:
    len = tao_get_buffer_contents(&srvbuf, &ptr);
    if (len > 0 && ((char*)ptr)[len-1] != '\n') {
        /* Append a newline for more readable output when xapset/xpaget are
            used from the command line. */
        (void)tao_put_char_to_buffer(NULL, &srvbuf, '\n');
        len = tao_get_buffer_contents(&srvbuf, &ptr);
    }
    *bufptr = ptr;
    *lenptr = len;
    status = 0;

 done:
    if (errs != TAO_NO_ERRORS) {
        tao_discard_errors(&errs);
    }
    if (argv != NULL) {
        free(argv);
    }
    return status;

 failure:
    status = -1;
    goto done;
}

/*---------------------------------------------------------------------------*/
/* SET CALLBACK */

typedef struct set_command set_command_t;
struct set_command {
    const char* name;
    int (*set)(XPA xpa, int argc, const char* argv[]);
};

static int xpaset_usage(XPA xpa, const char* argv0, const char* args)
{
    if (args == NULL) {
        xpa_error(xpa, "Usage: xpaset %s", argv0);
    } else {
        xpa_error(xpa, "Usage: xpaset %s %s", argv0, args);
    }
    return -1;
}

static int _set_debug(XPA xpa, int argc, const char* argv[])
{
    if (argc == 2 && strcmp(argv[1], "on") == 0) {
        debug = true;
    } else if (argc == 2 && strcmp(argv[1], "off") == 0) {
        debug = false;
    } else {
        xpaset_usage(xpa, argv[0], "on|off");
        return -1;
    }
    return 0;
}

static int _set_start(XPA xpa, int argc, const char* argv[])
{
    int nbufs;
    if (argc == 2) {
        if (tao_parse_int(argv[1], &nbufs, 10) != 0 || nbufs < 1) {
            xpa_error(xpa, "Bad number of acquisition buffers");
            return -1;
        }
    } else if (argc == 1) {
        nbufs = 4;
    } else {
        xpaset_usage(xpa, argv[0], NULL);
        return -1;
    }
    return send_command(xpa, COMMAND_START, nbufs);
}

static int _set_stop(XPA xpa, int argc, const char* argv[])
{
    if (argc != 1) {
        xpaset_usage(xpa, argv[0], NULL);
        return -1;
    }
    return send_command(xpa, COMMAND_STOP, 0);
}

static int _set_abort(XPA xpa, int argc, const char* argv[])
{
    if (argc != 1) {
        xpaset_usage(xpa, argv[0], NULL);
        return -1;
    }
    return send_command(xpa, COMMAND_ABORT, 0);
}

static int _set_quit(XPA xpa, int argc, const char* argv[])
{
    if (argc != 1) {
        xpaset_usage(xpa, argv[0], NULL);
        return -1;
    }
    quit = true;
    return send_command(xpa, COMMAND_EXIT, 0);
}

/* Yields the expected state, perhaps after waiting a bit, if no new
   commands arrive.  This function assumes that private resources have
   been locked by the caller. */
static state_t expected_state()
{
    if (priv->command == COMMAND_EXIT) {
        return STATE_DONE;
    }
    if (priv->state == STATE_STARTING || priv->state == STATE_ACQUIRING) {
        if (priv->command == COMMAND_STOP) {
            return STATE_STOPPING;
        }
        if (priv->command == COMMAND_ABORT) {
            return STATE_ABORTING;
        }
    }
    return priv->state;
}

static int _set_config(XPA xpa, int argc, const char* argv[])
{
    int status = 0;
    state_t state;
    const char* key;
    const char* val;
    andor_camera_config_t cfg, rec;
    bool set_xbin = false, set_ybin = false;
    bool set_xoff = false, set_yoff = false;
    bool set_width = false, set_height = false;
    bool set_exposuretime = false, set_framerate = false;

    if ((argc & 1) != 1) {
        xpaset_usage(xpa, argv[0], "key1 val1 [key2 val2 ...]");
        return -1;
    }
    memset(&rec, 0, sizeof(rec)); /* to avoid compiler warnings */
    for (int i = 1; i < argc - 1; i += 2) {
        key = argv[i];
        val = argv[i+1];
        if (debug) {
            fprintf(stderr, "key=%s, val=%s\n", key, val);
        }
        int c = key[0];
        if (c == 'e') {
            if (strcmp(key, "exposuretime") == 0) {
                if (set_exposuretime) {
                    goto duplicate;
                }
                if (tao_parse_double(val, &rec.exposuretime) != 0 ||
                    rec.exposuretime < 0) {
                    goto badvalue;
                }
                set_exposuretime = true;
                continue;
            }
        } else if (c == 'f') {
            if (strcmp(key, "framerate") == 0) {
                if (set_framerate) {
                    goto duplicate;
                }
                if (tao_parse_double(val, &rec.framerate) != 0 ||
                    rec.framerate <= 0) {
                    goto badvalue;
                }
                set_framerate = true;
                continue;
            }
        } else if (c == 'h') {
            if (strcmp(key, "height") == 0) {
                if (set_height) {
                    goto duplicate;
                }
                if (tao_parse_long(val, &rec.height, 10) != 0 ||
                    rec.height < 1) {
                    goto badvalue;
                }
                set_height = true;
                continue;
            }
        } else if (c == 'w') {
            if (strcmp(key, "width") == 0) {
                if (set_width) {
                    goto duplicate;
                }
                if (tao_parse_long(val, &rec.width, 10) != 0 ||
                    rec.width < 1) {
                    goto badvalue;
                }
                set_width = true;
                continue;
            }
        } else if (c == 'x') {
            if (strcmp(key, "xbin") == 0) {
                if (set_xbin) {
                    goto duplicate;
                }
                if (tao_parse_long(val, &rec.xbin, 10) != 0 || rec.xbin < 1) {
                    goto badvalue;
                }
                set_xbin = true;
                continue;
            }
            if (strcmp(key, "xoff") == 0) {
                if (set_xoff) {
                    goto duplicate;
                }
                if (tao_parse_long(val, &rec.xoff, 10) != 0 || rec.xoff < 0) {
                    goto badvalue;
                }
                set_xoff = true;
                continue;
            }
        } else if (c == 'y') {
            if (strcmp(key, "ybin") == 0) {
                if (set_ybin) {
                    goto duplicate;
                }
                if (tao_parse_long(val, &rec.ybin, 10) != 0 || rec.ybin < 0) {
                    goto badvalue;
                }
                set_ybin = true;
                continue;
            }
            if (strcmp(key, "yoff") == 0) {
                if (set_yoff) {
                    goto duplicate;
                }
                if (tao_parse_long(val, &rec.yoff, 10) != 0 || rec.yoff < 0) {
                    goto badvalue;
                }
                set_yoff = true;
                continue;
            }
        }
        xpa_error(xpa, "Unknown key `%s`", key);
        return -1;

    duplicate:
        xpa_error(xpa, "Duplicate key `%s`", key);
        return -1;

    badvalue:
        xpa_error(xpa, "Invalid value for key `%s`", key);
        return -1;
    }

    lock_mutex(priv);
    while (true) {
        state = expected_state();
        if (debug) {
            fprintf(stderr, "state: %s\n", state_name(state));
        }
        if (state == STATE_SLEEPING) {
            /* Get current configuration, apply the changes, make sure the
	       configuration is up-to-date and whatever could have been done,
	       reflect the actual configuration into the shared camera
	       data. */
	    andor_get_configuration(priv->cam, &cfg);
#define SETOPT(opt) if (set_##opt) cfg.opt = rec.opt
	    SETOPT(xbin);
	    SETOPT(ybin);
	    SETOPT(xoff);
	    SETOPT(yoff);
	    SETOPT(width);
	    SETOPT(height);
	    SETOPT(exposuretime);
	    SETOPT(framerate);
#undef SETOPT
	    if (andor_set_configuration(priv->cam, &cfg) != 0) {
		status = -1;
	    }
	    if (andor_update_configuration(priv->cam, true) != 0) {
		status = -1;
	    }
	    tao_lock_shared_camera(NULL, srvcam->shared);
	    andor_reflect_configuration(srvcam->shared, priv->cam);
	    tao_unlock_shared_camera(NULL, srvcam->shared);
            break;
        } else if (state == STATE_STOPPING || state == STATE_ABORTING) {
            /* Acquisition is about to stop.  Manage to wait a bit (one
               millisecond) before checking again. */
            unlock_mutex(priv);
            (void)tao_sleep(0.001);
            lock_mutex(priv);
        } else {
            status = -1;
            break;
        }
    }
    unlock_mutex(priv);

    if (status != 0) {
        /* Deal with errors. */
        if (state == STATE_STARTING || state == STATE_ACQUIRING) {
            xpa_error(xpa, "Cannot change settings during acquisition");
        } else if (state == STATE_DONE) {
            xpa_error(xpa, "Camera has been closed");
        } else {
            report_errors(xpa, &priv->cam->errs);
        }
    }
    return status;
}

static set_command_t setcmds[] = {
#define SETCMD(attr) {#attr, _set_##attr}
     SETCMD(abort),
     SETCMD(config),
     SETCMD(debug),
     SETCMD(quit),
     SETCMD(start),
     SETCMD(stop),
#undef SETCMD
     { NULL, NULL }
};

/* Callback to answer an XPASet request. */
static int recv_callback(void* recv_data, void* call_data,
                         char* command, char* buf, size_t len)
{
    XPA xpa = (XPA)call_data;
    tao_error_t* errs = TAO_NO_ERRORS;
    int c, argc, status;
    const char** argv = NULL;

    /* Split command in a list of arguments. */
    if (debug) {
        fprintf(stderr, "recv: %s [%ld byte(s)]\n", command, (long)len);
    }
    argc = tao_split_command(&errs, &argv, command, -1);
    if (argc == 0) {
        /* No command. */
        status = 0;
        goto done;
    } else if (argc < 0) {
        report_errors(xpa, &errs);
        status = -1;
        goto done;
    }
    if (debug) {
        for (int i = 0; i < argc; ++i) {
            fprintf(stderr, "send: [%d] >>%s<<\n", i, argv[i]);
        }
    }

    /* Commands take no data, check this. */
    if (len > 0) {
        XPAError(xpa, "Expecting no data");
        status = -1;
        goto done;
    }

    /* Execute the command. */
    c = argv[0][0];
    for (int i = 0; true; ++i) {
        const char* name = setcmds[i].name;
        if (name == NULL) {
            xpa_error(xpa, "Unknown parameter for xpaset command");
            status = -1;
            break;
        }
        if (name[0] == c && strcmp(argv[0], name) == 0) {
            status = setcmds[i].set(xpa, argc, argv);
            break;
        }
    }

    /* Free resources. */
 done:
    if (errs != TAO_NO_ERRORS) {
        tao_discard_errors(&errs);
    }
    if (argv != NULL) {
        free(argv);
    }
    return status;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
    XPA srv;
    char* serverclass = "TAO";
    char servername[64];
    char* send_mode = "acl=true,freebuf=false";
    char* recv_mode = "";
    void* send_data = NULL;
    void* recv_data = NULL;
    andor_camera_t* cam = NULL;
    andor_camera_config_t cfg;
    bool optional = true;
    long dev = -1, ndevices;
    int msec = 10000; /* timeout in milliseconds for XPAPoll */
    int status;
    int nframes = 20;
    int perms = 0644;

    /* Initializations. */
    tao_initialize_static_buffer(&srvbuf);

    /* Get the number of Andor devices.  This also initializes the
       library. Since we use NULL for error reporting, any errors will be
       fatal, hence there is no needs to check the result. */
    ndevices = andor_get_ndevices(NULL);

    /* Parse arguments. */
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            /* Arguments can no longer be options. */
            optional = false;
        }
        if (optional && strcmp(argv[i], "-debug") == 0) {
            debug = true;
        } else if (optional && strcmp(argv[i], "--") == 0) {
            optional = false;
        } else if (cam == NULL) {
            char* end;
            dev = strtol(argv[i], &end, 0);
            if (end == argv[i] || *end != '\0') {
                goto usage;
            }
            if (dev < 0 || dev >= ndevices) {
                fprintf(stderr, "%s: Invalid device number %ld\n",
                        progname, dev);
                return EXIT_FAILURE;
            }
        } else {
        usage:
            fprintf(stderr, "Usage: %s [OPTIONS] [--] [dev]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (dev < 0) {
        goto usage;
    }

    /* Create the master camera. */
    srvcam = tao_create_camera(NULL, nframes, perms);

    /* Open the camera device.  Since we use NULL for error reporting, any
       errors will be fatal, hence there is no needs to check the result. */
    cam = andor_open_camera(NULL, dev);

    /* Apply initial configuration.  Make sure to synchronize the actual
       configuration after any changes in case some parameters are not exactly
       the requested ones.  */
    if (andor_update_configuration(cam, true) != 0) {
        andor_report_errors(cam);
        return EXIT_FAILURE;
    }
    andor_get_configuration(cam, &cfg);
#if 0
    if (andor_set_configuration(cam, &cfg) != 0) {
        andor_report_errors(cam);
        return EXIT_FAILURE;
    }
    andor_get_configuration(cam, &cfg);
#endif
    if (debug) {
        andor_print_camera_configuration(stdout, cam);
    }
    andor_reflect_configuration(srvcam->shared, cam);

    /* Allocate other private resources.  Any error is fatal. */
    priv = create_private_data(cam);
    if (priv == NULL) {
        andor_report_errors(cam);
        return EXIT_FAILURE;
    }
    priv->proc = process_frame;

    /* Start the image processing thread. */
    status = pthread_create(&priv->worker, NULL,
                            run_acquisition, (void*)priv);
    if (status != 0) {
        tao_push_error(NULL, "pthread_create", status);
        return EXIT_FAILURE;
    }
    sprintf(servername, "andor%d", (int)dev);
    srv = XPANew(serverclass, servername, "some help",
                 send_callback, send_data, send_mode,
                 recv_callback, recv_data, recv_mode);
    if (srv == NULL) {
        fprintf(stderr, "failed to start XPA server %s:%s\n",
                serverclass, servername);
        exit(1);
    }

    /*
     * We do not use XPAMainLoop() because we want to exit when the `quit`
     * command is sent.
     */
    while (! quit) {
        XPAPoll(msec, 1);
    }
    if (XPAFree(srv) != 0) {
        fprintf(stderr, "failed to remove XPA server %s:%s\n",
                serverclass, servername);
        exit(1);
    }

    /* Free all resources (in reverse order compared t creation). */
    XPACleanup();
    tao_finalize_buffer(&srvbuf);
    destroy_private_data(priv);
    priv = NULL;
    andor_close_camera(cam);
    (void)tao_finalize_camera(NULL, srvcam);
    tao_finalize_buffer(&srvbuf);
    return EXIT_SUCCESS;
}
