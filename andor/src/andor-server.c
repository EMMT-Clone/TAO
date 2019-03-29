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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <xpa.h>
#include <andor.h>
#include <tao.h>

static const char* progname = "andor-server";


/* These are the tasks that can be scheduled.  The policy is as follows: the
   master must lock the resources to schedule a new task, EXIT is irreversible,
   after a START, any of STOP, ABORT and EXIT are possible. */
typedef enum {
    TASK_NONE,  /* Nothing to do */
    TASK_START, /* Request to start */
    TASK_STOP,  /* Request to stop */
    TASK_ABORT, /* Request to abort */
    TASK_EXIT,  /* Request to exit */
} pending_task_t;

/* These are the tasks that are currently been executed.  The worker thread,
   accounts for a new scheduled task when sleeping and a notifification is
   signaled or after every acquired frame (or timeout while waiting for a new
   frame).  When a new scheduled task is taken into account by the worker
   thread, it changes the value of its `running_task` parameter and when the
   task is completed,it changes the value of its `pending_task` parameter. */
typedef enum {
    TASK_SLEEPING,  /* Sleeping */
    TASK_STARTING,  /* Starting acquisition */
    TASK_ACQUIRING, /* Acquisition running */
    TASK_STOPPING,  /* Stopping acquisition */
    TASK_ABORTING,  /* Aborting acquisition */
    TASK_EXITING,   /* Exiting */
} running_task_t;

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
   variables could have been used) but this is cleaner and helps to clarify the
   code. */
typedef struct shared_data shared_data_t;
struct shared_data {
    pthread_mutex_t mutex;
    pthread_cond_t notify;
    pthread_t worker;
    andor_camera_t* cam;
    pending_task_t pending_task;
    running_task_t running_task;
    processor *proc;
    void* data;
    double timeout;
    int nbufs;
    long timeouts;   /* Number of timouts so far */
    long frames;    /* Number of frames so far */
};

static shared_data_t*
create_shared_data(andor_camera_t* cam)
{
    shared_data_t* shared;

    shared = (shared_data_t*)tao_calloc(&cam->errs, 1, sizeof(shared_data_t));
    if (shared == NULL) {
        return NULL;
    }
    if (tao_initialize_mutex(&cam->errs, &shared->mutex, false) != 0) {
        free((void*)shared);
        return NULL;
    }
    if (tao_initialize_condition(&cam->errs, &shared->notify) != 0) {
        (void)tao_destroy_mutex(&cam->errs, &shared->mutex, true);
        free((void*)shared);
        return NULL;
    }
    shared->cam = cam;
    shared->pending_task = TASK_NONE;
    shared->running_task = TASK_SLEEPING;
    shared->proc = NULL;
    shared->data = NULL;
    shared->timeout = 0.1;
    shared->nbufs = 4;
    shared->timeouts = 0;
    shared->frames = 0;
    return shared;
}

static void
destroy_shared_data(shared_data_t* shared)
{
    if (shared != NULL) {
        (void)tao_destroy_mutex(NULL, &shared->mutex, true);
        (void)tao_destroy_condition(NULL, &shared->notify);
        free((void*)shared);
    }
}

int
andor_cget(andor_camera_t* cam,
           const char* opt, tao_buffer_t* buf)
{
    andor_camera_config_t* cfg = &cam->config;
    long len = tao_strlen(opt);

    tao_clear_buffer(buf);
    if (len >= 1 && opt[0] == '-') {
        --len;
        ++opt;
    }
    if (len > 0) {
#define GETOPT(str, field, format) GETOPT1(#field, str, field, format)
#define GETOPT1(name, str, field, format)                               \
        do {                                                            \
            if (strcmp(opt, name) == 0) {                               \
                return tao_print_to_buffer(&cam->errs, buf,             \
                                           format, str->field);         \
            }                                                           \
        } while (false)
        switch (opt[0]) {
        case 'e':
            GETOPT(cfg, exposuretime, "%.6f");
#if 0
            /* FIXME: use names? */
            if (strcmp(opt, "encodings") == 0) {
            }
#endif
            break;
        case 'f':
            GETOPT(cfg, framerate, "%g");
            break;
        case 'h':
            GETOPT(cfg, height, "%ld");
            break;
        case 'p':
            /* FIXME: use names or indices? */
            GETOPT(cfg, pixelencoding, "%d");
            break;
        case 's':
            GETOPT(cam, sensorheight, "%ld");
            GETOPT(cam, sensorwidth, "%ld");
            GETOPT(cam, state, "%d");
            break;
        case 't':
            GETOPT(cfg, temperature, "%.1f");
            break;
        case 'w':
            GETOPT(cfg, width, "%ld");
            break;
        case 'x':
            GETOPT(cfg, xbin, "%ld");
            GETOPT(cfg, xoff, "%ld");
            break;
        case 'y':
            GETOPT(cfg, ybin, "%ld");
            GETOPT(cfg, yoff, "%ld");
            break;
        }
    }
    andor_push_error(cam, __func__, TAO_BAD_NAME);
    return -1;
}

/* The following macros or private functions are to simplify the code, they all
   assume that errors are unrecoverable. */
#define get_monotonic_time(ts) tao_get_monotonic_time(NULL, ts)
#define lock_mutex(shared) tao_lock_mutex(NULL, &shared->mutex)
#define unlock_mutex(shared) tao_unlock_mutex(NULL, &shared->mutex)
#define trylock_mutex(shared) tao_trylock_mutex(NULL, &shared->mutex)
#define signal_condition(shared) tao_signal_condition(NULL, &shared->cond)
#define wait_condition(shared) tao_wait_condition(NULL, &shared->notify, \
                                                  &shared->mutex)
#define fatal_error(shared)                             \
    do {                                                \
        andor_report_errors(shared->cam);               \
        if (shared->cam->state == 2) {                  \
            (void)andor_stop_acquisition(shared->cam);  \
        }                                               \
        exit(EXIT_FAILURE);                             \
    } while (false)
#define die_if(expr, shared)                    \
    do {                                        \
        if (expr) {                             \
            fatal_error(shared);                \
        }                                       \
    } while (false)
#define start_acquisition(shared) \
    die_if(andor_start_acquisition(shared->cam, shared->nbufs) != 0, shared)
#define queue_buffer(shared, buf, siz) \
    die_if(andor_queue_buffer(shared->cam, buf, siz) != 0, shared)
#define stop_acquisition(shared) \
    die_if(andor_stop_acquisition(shared->cam) != 0, shared)

/* This is the function executed by the image processing thread. */
static void* run_acquisition(void* arg)
{
    shared_data_t* shared = (shared_data_t*)arg;
    struct timespec ts;
    int status;

    lock_mutex(shared);
    shared->running_task = TASK_SLEEPING;
    while (shared->pending_task != TASK_EXIT) {

        /* Wait for a new task to be scheduled taking care of spurious
           wake-ups. */
        while (shared->pending_task == TASK_NONE) {
            wait_condition(shared);
        }

        if (shared->pending_task == TASK_START) {
            /* Start acquisition.  Since starting can take some time, it is
               checked whether a new task has been scheduled in the meantime.
               FIXME: signal the caller when starting completes? */
            shared->running_task = TASK_STARTING;
            if (shared->cam->state == 1) {
                unlock_mutex(shared);
                start_acquisition(shared);
                lock_mutex(shared);
            }
            if (shared->pending_task == TASK_NONE ||
                shared->pending_task == TASK_START) {
                /* No new task has been scheduled while starting.  Indicate
                   that start was done and that we are now acquiring. */
                shared->pending_task = TASK_NONE;
                shared->running_task = TASK_ACQUIRING;
            }
        }

        /* Deal with other scheduled tasks.  This group of tests must be
           independent from the previous (not and `else if` statement) because
           a new task may have been scheduled while starting. */
        if (shared->pending_task == TASK_STOP ||
            shared->pending_task == TASK_ABORT) {
            shared->running_task = (shared->pending_task == TASK_STOP ?
                                    TASK_STOPPING : TASK_ABORTING);
            if (shared->cam->state == 2) {
                unlock_mutex(shared);
                stop_acquisition(shared);
                lock_mutex(shared);
            }
            if (shared->pending_task == TASK_NONE ||
                shared->pending_task == TASK_STOP ||
                shared->pending_task == TASK_ABORT) {
                /* No new task has been scheduled while stopping.  Indicate
                   that acquiring was stopped and that we are now sleeping. */
                shared->pending_task = TASK_NONE;
                shared->running_task = TASK_SLEEPING;
            }
        }

        /* Acquisition loop. */
        while (shared->running_task == TASK_ACQUIRING) {
            /* Wait for new acquisition buffer and get time of arrival. */
            void* buf;
            long siz;
            unlock_mutex(shared);
            status = andor_wait_buffer(shared->cam, &buf, &siz,
                                       shared->timeout);
            if (status == -1) {
                /* An error occured. */
                fatal_error(shared);
            }
            get_monotonic_time(&ts);

            /* Check whether a new command was scheduled while waiting, then
               perform any relevant action.  The mutex is loked on entry,
               unlocked when done (unlocking is done in different places). */
            lock_mutex(shared);
            switch (shared->pending_task) {
            case TASK_NONE:
                break;
            case TASK_START:
                /* Just ignore re-start. */
                shared->pending_task = TASK_NONE;
                shared->running_task = TASK_ACQUIRING;
                break;
            case TASK_STOP:
                shared->running_task = TASK_STOPPING;
                break;
            case TASK_ABORT:
                shared->running_task = TASK_ABORTING;
                break;
            case TASK_EXIT:
                shared->running_task = TASK_EXITING;
                break;
            }
            if (shared->running_task == TASK_ACQUIRING ||
                shared->running_task == TASK_STOPPING) {
                /* Deal with the new acquired frame. */
                if (status == 0) {
                    /* A timeout occured. */
                    ++shared->timeouts;
                    unlock_mutex(shared);
                } else {
                    /* A new acquisition buffer is available.  Apply
                       processing if any, then re-queue the buffer. */
                    processor *proc;
                    void* data;
                    proc = shared->proc;
                    data = shared->data;
                    unlock_mutex(shared);
                    if (proc != NULL) {
                        proc(data, &ts, buf, siz);
                    }
                    queue_buffer(shared, buf, siz);
                }
            } else {
                lock_mutex(shared);
            }
        } /* end of acquisition loop */
    }
    shared->running_task = TASK_EXITING;
    unlock_mutex(shared);
    return NULL;
}

/* Callback to answer an XPAGet request. */
static int send_callback(void* send_data, void* call_data,
                         char* command, char** bufptr, size_t* lenptr)
{
    return 0;
}

/* Callback to answer an XPASet request. */
static int recv_callback(void* recv_data, void* call_data,
                         char* command, char* buf, size_t len)
{
    return 0;
}

int main(int argc, char* argv[])
{
    XPA srv;
    char* serverclass = "TAO";
    char* servername = "virtualcamera1";
    char* send_mode = "acl=true,freebuf=false";
    char* recv_mode = "";
    void* send_data = NULL;
    void* recv_data = NULL;
    andor_camera_t* cam = NULL;
    shared_data_t* shared = NULL;
    andor_camera_config_t cfg;
    bool optional = true, debug = false;
    long dev = -1, ndevices;
    int msec = 10;
    int status;
    bool quit = false;
    tao_buffer_t srvbuf;

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

    /* Open the camera.  Since we use NULL for error reporting, any errors will
       be fatal, hence there is no needs to check the result. */
    cam = andor_open_camera(NULL, dev);

    /* Apply initial configuration.  Make sure to synchronize the actual
       configuration after any changes in case some parameters are not exactly
       the requested ones.  */
    andor_update_configuration(cam, true);
    andor_get_configuration(cam, &cfg);
    //cfg.xbin = 1;
    //cfg.ybin = 1;
    //cfg.xoff = 300;
    //cfg.yoff = 200;
    //cfg.width = 640;
    //cfg.height = 480;
    //cfg.framerate = 40.0;
    //cfg.exposuretime = 0.005;
    //cfg.pixelencoding = ANDOR_ENCODING_MONO12PACKED;
    if (andor_set_configuration(cam, &cfg) != 0) {
        andor_report_errors(cam);
        return EXIT_FAILURE;
    }
    andor_get_configuration(cam, &cfg);
    if (debug) {
        andor_print_camera_configuration(stdout, cam);
    }

    /* Allocate other shared resources.  Any error is fatal. */
    shared = create_shared_data(cam);
    if (shared == NULL) {
        andor_report_errors(cam);
        return EXIT_FAILURE;
    }

    /* Start the image processing thread. */
    status = pthread_create(&shared->worker, NULL,
                            run_acquisition, (void*)shared);
    if (status != 0) {
        tao_push_error(NULL, "pthread_create", status);
        return EXIT_FAILURE;
    }


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
    destroy_shared_data(shared);
    andor_close_camera(cam);
    tao_finalize_buffer(&srvbuf);
    return EXIT_SUCCESS;
}
