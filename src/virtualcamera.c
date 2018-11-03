#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <xpa.h>
#include <tao.h>
#include <tao-private.h>


/* XPA routines are not thread safe, the XPA server must be run in a given
 * single thread.  The following static data are only available for the
 * server routines. */
static int debug = 1;
static int quit = 0;
static tao_buffer_t srvbuf; /* dynamic i/o buffer for sending messages to the
                             * client */

#define FULLWIDTH  640
#define FULLHEIGHT 480

#if 0 /* FIXME: unused */
static const double max_rate = 2e3;
static const double min_rate = 1.0;
static const double min_exposure = 0.0;
static const double max_exposure = 1.0;
#endif

static tao_camera_t* cam = NULL;

static int send_callback(void* send_data, void* call_data,
                         char* command, char** bufptr, size_t* lenptr);

static int recv_callback(void* recv_data, void* call_data,
                         char* command, char* buf, size_t len);

/* Report TAO errors to XPA client. */
static void report_error(tao_error_t** errs, XPA xpa);

/*---------------------------------------------------------------------------*/
/* GENERATE IMAGES */

#if 0
static void* raw_img = NULL;
static size_t raw_size = 0;

static void*
resize_image(int depth, int width, int height)
{
    size_t elsize = (depth <= 8 ? 1 : 2);
    size_t minsize = elsize*width*height;
    size_t maxsize = minsize + (minsize >> 1);
    if (raw_img == NULL || raw_size < minsize || raw_size > maxsize) {
        if (raw_img != NULL) {
            free(raw_img);
        }
        raw_img = tao_malloc(NULL, maxsize);
        raw_size = maxsize;
    }
    return raw_img;
}

static void*
generate_image(int depth, int width, int height, uint32_t bits)
{
    void* raw = resize_image(depth, width, height);

#define GENERATE(TYPE)                                  \
    do {                                                \
        TYPE xmsk = ( bits        & 0xFF);              \
        TYPE ymsk = ((bits >>  8) & 0xFF);              \
        TYPE xoff = ((bits >> 16) & 0xFF);              \
        TYPE yoff = ((bits >> 24) & 0xFF);              \
        for (int y = 0; y < height; ++y) {              \
            TYPE* line = ((TYPE*)raw) + y*width;        \
            TYPE mask = ((y + yoff) & ymsk);            \
            for (int x = 0; x < width; ++x) {           \
                line[x] = mask & ((x + xoff) & xmsk);   \
            }                                           \
        }                                               \
    } while (0)

    if (depth <= 8) {
        GENERATE(uint8_t);
    } else {
        GENERATE(uint16_t);
    }

#undef GENERATE

    return raw;
}
#endif

#define GENERATE(FUNC, TYPE)                                    \
    static void                                                 \
    FUNC(TYPE* dest, int width, int height, uint32_t bits)      \
    {                                                           \
        TYPE xmsk = 0xFF;                                       \
        TYPE xoff = ( bits        & 0xFF);                      \
        TYPE yoff = ((bits >>  8) & 0xFF);                      \
        for (int y = 0; y < height; ++y) {                      \
            TYPE* line = ((TYPE*)dest) + y*width;               \
            TYPE mask = ((y + yoff) & 0x0F) == 0x0F ? 0 : 0xFF; \
            for (int x = 0; x < width; ++x) {                   \
                line[x] = mask & ((x + xoff) & xmsk);           \
            }                                                   \
        }                                                       \
    }
GENERATE(generate_u8,  uint8_t);
GENERATE(generate_u16, uint16_t);

#undef GENERATE

static void produce_image(unsigned bits)
{
    tao_error_t* errs = TAO_NO_ERRORS;
    tao_time_t t0, t1, dt;
    char buf[32];
    tao_shared_array_t* arr = NULL;

    if (tao_get_monotonic_time(&errs, &t0) != 0) {
        goto error;
    }

    /*
     * Lock the camera and get an array to write the captured image data.  Note
     * that, to avoid deadlocks, the camera is unlocked prior to check whether
     * the returned array is valid.
     */
    if (tao_lock_shared_camera(&errs, cam->shared) == 0) {
        if (cam->shared->depth <= 8) {
            cam->shared->depth = 8;
            cam->shared->pixel_type = TAO_UINT8;
        } else {
            cam->shared->depth = 16;
            cam->shared->pixel_type = TAO_UINT16;
        }
        arr = tao_fetch_next_frame(&errs, cam);
        tao_unlock_shared_camera(&errs, cam->shared);
    }
    if (errs != TAO_NO_ERRORS) {
        goto error;
    }

    /* Generate/process the image while the shared camera data is unlocked. */
    tao_set_shared_array_timestamp(arr, t0.s, t0.ns);
    void* data = tao_get_shared_array_data(arr);
    int width = tao_get_shared_array_size(arr, 1);
    int height = tao_get_shared_array_size(arr, 2);
    if (cam->shared->pixel_type == TAO_UINT8) {
        generate_u8(data, width, height, bits);
    } else if (cam->shared->pixel_type == TAO_UINT16) {
        generate_u16(data, width, height, bits);
    }
    if (debug) {
        if (tao_get_monotonic_time(&errs, &t1) != 0) {
            goto error;
        }
        tao_subtract_times(&dt, &t1, &t0);
        tao_sprintf_time(buf, &dt);
        fprintf(stderr, "%d×%d image (ident=%d) generated in %s seconds\n",
                width, height, tao_get_shared_array_ident(arr), buf);
    }

    /* Re-lock the shared camera data and publish image. */
    if (tao_lock_shared_camera(&errs, cam->shared) == 0) {
        tao_publish_next_frame(&errs, cam, arr);
        tao_unlock_shared_camera(&errs, cam->shared);
    }

 error:
    tao_report_errors(&errs);
}

/*---------------------------------------------------------------------------*/
/* ERROR MESSAGES */

/*
 * The following buffer is used for storing answers or small error messages
 * sent by the server.  FIXME: use srvbuf?
 */
static char message[1024];

static void
tao_get_error_details(char* buffer, int code, const char** reason,
                      const char** info, tao_error_getter_t* proc)
{
    if (proc != NULL) {
        /* Use callback to retrieve error details. */
        *reason = NULL;
        *info = NULL;
        proc(code, reason, info);
    } else {
        /* Assume a system error or a TAO error. */
        *reason = tao_get_error_reason(code);
        *info = tao_get_error_name(code);
        if (*info != NULL) {
            if (code > 0) {
                if (strcmp(*info, "UNKNOWN_SYSTEM_ERROR") == 0) {
                    *info = NULL;
                }
            } else if (code < 0) {
                if (strcmp(*info, "UNKNOWN_ERROR") == 0) {
                    *info = NULL;
                }
            }
        }
    }
    if (*reason == NULL) {
        *reason = "Some error occured";
    }
    if (*info == NULL) {
        /* Use the numerical value of the error code. */
        sprintf(buffer, "%d", code);
        *info = buffer;
    }
}

static void report_error(tao_error_t** errs, XPA xpa)
{
    const char* func;
    const char* reason;
    const char* info;
    tao_error_getter_t* proc;
    int code, flag = 1;
    char* buf;
    char infobuf[20];

    /* Write error message. */
    tao_clear_buffer(&srvbuf);
    while (tao_pop_error(errs, &func, &code, &proc)) {
        /*
         * Append next error message to the i/o buffer.  A fatal error results
         * in case of failure here.
         */
        tao_get_error_details(infobuf, code, &reason, &info, proc);
        if (debug) {
            fprintf(stderr, "%s %s in function `%s` [%s]\n",
                    (flag ? "{ERROR}" : "       "), reason, func, info);
        }
        tao_print_to_buffer(NULL, &srvbuf, "%s%s in function `%s` [%s]\n",
                            (flag ? "" : "; "), reason, func, info);
        flag = 0;
    }
    if (flag) {
        (void)tao_get_buffer_contents(&srvbuf, (void**)&buf);
    } else {
        /* There were no errors! */
        buf = "no errors!";
    }
    XPAError(xpa, buf);
}


/*
 * When printing error messages to a fixed size buffer, it is important to only
 * print the arguments which are correct (to avoid buffer overflow).  In the
 * routines below `argc` is the number of arguments to print.
 */

static void syntax_error(XPA xpa, char* buf, const char* prefix,
                         int argc, const char** argv, const char* suffix)
{
    size_t len = strlen(prefix);
    if (buf != prefix) {
        strcpy(buf, prefix);
    }
    for (int i = 0; i < argc && argv[i] != NULL; ++i) {
        if (i > 0) {
            buf[len] = ' ';
            ++len;
        }
        strcpy(buf + len, argv[i]);
        len += strlen(buf + len);
    }
    strcpy(buf + len, suffix);
    XPAError(xpa, buf);
}

static void too_few_arguments(XPA xpa, char* buf, const char* cmd,
                              int argc, const char** argv)
{
    sprintf(buf, "missing argument(s) in %s `", cmd);
    syntax_error(xpa, buf, buf, argc, argv, " ...` command");
}

static void too_many_arguments(XPA xpa, char* buf, const char* cmd,
                               int argc, const char** argv)
{
    sprintf(buf, "unexpected argument(s) after %s `", cmd);
    syntax_error(xpa, buf, buf, argc, argv, " ...` command");
}

static void invalid_arguments(XPA xpa, char* buf, const char* cmd,
                              int argc, const char** argv)
{
    sprintf(buf, "invalid argument(s) in %s `", cmd);
    syntax_error(xpa, buf, buf, argc, argv, " ...` command");
}

/*---------------------------------------------------------------------------*/

static void start_command(int nbufs) {}
static void abort_command() {}
static void stop_command() {}
static void close_command() {}

#define CHECK_MIN_ARGC(nprt, nmin)                              \
    do {                                                        \
        if (argc < nmin) {                                      \
            too_few_arguments(xpa, message, CMD, nprt, argv);   \
            goto error;                                         \
        }                                                       \
    } while (0)
#define CHECK_ARGC(nprt, nmin, nmax)                            \
    do {                                                        \
        if (argc < nmin) {                                      \
            too_few_arguments(xpa, message, CMD, nprt, argv);   \
            goto error;                                         \
        }                                                       \
        if (argc > nmax) {                                      \
            too_many_arguments(xpa, message, CMD, nprt, argv);  \
            goto error;                                         \
        }                                                       \
    } while (0)

#define INVALID_ARGUMENTS(n)                            \
    do {                                                \
        invalid_arguments(xpa, message, CMD, n, argv);  \
        goto error;                                     \
    } while (0)

/* Callback to answer an XPAGet request. */
#define CMD "get"
static int send_callback(void* send_data, void* call_data,
                         char* command, char** bufptr, size_t* lenptr)
{
    /* Automatic variables. */
    XPA xpa = (XPA)call_data;
    tao_error_t* errs = TAO_NO_ERRORS;
    int argc, status = 0;
    size_t len;
    const char** argv = NULL;

    if (debug) {
        fprintf(stderr, "send: %s\n", command);
    }
    argc = tao_split_command(&errs, &argv, command, -1);
    if (argc < 0) {
        goto error;
    }
    if (debug) {
        for (int i = 0; i < argc; ++i) {
            fprintf(stderr, "send: [%d] >>%s<<\n", i, argv[i]);
        }
    }

    /* Parse command starting with an empty answer. */
    message[0] = '\0';
    if (argc < 1) {
        goto done;
    }
    int c = argv[0][0];
    if (c == 'b' && strcmp(argv[0], "bias") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%.1f", cam->shared->bias);
    } else if (c == 'd' && strcmp(argv[0], "debug") == 0) {
        CHECK_ARGC(1, 1, 1);
        strcpy(message, (debug ? "on" : "off"));
    } else if (c == 'e' && strcmp(argv[0], "exposure") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%.6f", cam->shared->exposure);
    } else if (c == 'f' && strcmp(argv[0], "fullheight") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d", cam->shared->fullheight);
    } else if (c == 'f' && strcmp(argv[0], "fullwidth") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d", cam->shared->fullwidth);
    } else if (c == 'g' && strcmp(argv[0], "gamma") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%.3f", cam->shared->gamma);
    } else if (c == 'g' && strcmp(argv[0], "gain") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%.1f", cam->shared->gain);
    } else if (c == 'h' && strcmp(argv[0], "height") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d", cam->shared->height);
    } else if (c == 'p' && strcmp(argv[0], "ping") == 0) {
        tao_time_t ts;
        CHECK_ARGC(1, 1, 1);
        if (tao_get_monotonic_time(&errs, &ts) != 0) {
            goto error;
        }
        tao_sprintf_time(message, &ts);
    } else if (c == 'r' && strcmp(argv[0], "rate") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%.3f", cam->shared->rate);
    } else if (c == 'r' && strcmp(argv[0], "roi") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d %d %d %d",
                cam->shared->xoff, cam->shared->yoff,
                cam->shared->width, cam->shared->height);
    } else if (c == 's' && strcmp(argv[0], "shmid") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d", tao_get_shared_camera_ident(cam->shared));
    } else if (c == 's' && strcmp(argv[0], "state") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d", cam->shared->state);
    } else if (c == 'w' && strcmp(argv[0], "width") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d", cam->shared->width);
    } else if (c == 'x' && strcmp(argv[0], "xoff") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d", cam->shared->xoff);
    } else if (c == 'y' && strcmp(argv[0], "yoff") == 0) {
        CHECK_ARGC(1, 1, 1);
        sprintf(message, "%d", cam->shared->yoff);
    } else {
        XPAError(xpa, "unknown parameter for get `...` command");
        goto error;
    }

 done:
    len = strlen(message);
    if (len > 0) {
        /*
         * Append a newline for more readable output when xapset/xpaget are
         * used from the command line.
         */
        message[len] = '\n';
        message[len+1] = '\0';
    }
    *bufptr = message;
    *lenptr = len + 1;
 cleanup:
    if (errs != TAO_NO_ERRORS) {
        /* FIXME: This should not happen, at least print a warning. */
        tao_discard_errors(&errs);
    }
    if (argv != NULL) {
        free(argv);
    }
    return status;

 error:
    if (errs != TAO_NO_ERRORS) {
        report_error(&errs, xpa);
    }
    status = -1;
    goto cleanup;
}


/* Callback to answer an XPASet request. */
#undef CMD
#define CMD "set"
static int recv_callback(void* recv_data, void* call_data,
                         char* command, char* buf, size_t len)
{
    XPA xpa = (XPA)call_data;
    tao_error_t* errs = TAO_NO_ERRORS;
    int argc, status = 0;
    const char** argv = NULL;

    if (debug) {
        fprintf(stderr, "recv: %s [%ld byte(s)]\n", command, (long)len);
    }
    argc = tao_split_command(&errs, &argv, command, -1);
    if (argc < 0) {
        report_error(&errs, xpa);
        goto error;
    }
    if (debug) {
        for (int i = 0; i < argc; ++i) {
            fprintf(stderr, "recv: [%d] >>%s<<\n", i, argv[i]);
        }
    }

    /* Commands take no data, check this. */
    if (len > 0) {
        XPAError(xpa, "expecting no data");
        goto error;
    }

    /* Parse command. */
    if (argc < 1) {
        goto done;
    }
    int c = argv[0][0];
    if (c == 'a' && strcmp(argv[0], "abort") == 0) {
        CHECK_ARGC(1, 1, 1);
        if (cam->shared->state == 2) {
            abort_command();
            cam->shared->state = 1;
        } else {
            XPAError(xpa, "no acquisition is running");
            goto error;
        }
    } else if (c == 'b' && strcmp(argv[0], "bias") == 0) {
        double bias;
        CHECK_ARGC(1, 2, 2);
        if (tao_parse_double(argv[1], &bias) != 0) {
            XPAError(xpa, "bad value for `bias`");
            goto error;
        }
        if (cam->shared->state >= 2) {
            XPAError(xpa, "cannot change `bias` during an acquisition");
            goto error;
        }
        cam->shared->bias = bias;
    } else if (c == 'd' && strcmp(argv[0], "debug") == 0) {
        CHECK_ARGC(1, 2, 2);
        if (strcmp(argv[1], "on") == 0) {
            debug = 1;
        } else if (strcmp(argv[1], "off") == 0) {
            debug = 0;
        } else {
            INVALID_ARGUMENTS(1);
        }
    } else if (c == 'g' && strcmp(argv[0], "gain") == 0) {
        double gain;
        CHECK_ARGC(1, 2, 2);
        if (tao_parse_double(argv[1], &gain) != 0 || gain <= 0) {
            XPAError(xpa, "bad value for `gain`");
            goto error;
        }
        if (cam->shared->state >= 2) {
            XPAError(xpa, "cannot change `gain` during an acquisition");
            goto error;
        }
        cam->shared->gain = gain;
    } else if (c == 'g' && strcmp(argv[0], "gamma") == 0) {
        double gamma;
        CHECK_ARGC(1, 2, 2);
        if (tao_parse_double(argv[1], &gamma) != 0 || gamma <= 0 ||
            gamma > 2) {
            XPAError(xpa, "bad value for `gamma`");
            goto error;
        }
        if (cam->shared->state >= 2) {
            XPAError(xpa, "cannot change `gamma` during an acquisition");
            goto error;
        }
        cam->shared->gamma = gamma;
    } else if (c == 'p' && strcmp(argv[0], "produce") == 0) {
        CHECK_ARGC(1, 1, 1);
        produce_image(0x3256ae07);
    } else if (c == 'q' && strcmp(argv[0], "quit") == 0) {
        CHECK_ARGC(1, 1, 1);
        if (cam->shared->state == 2) {
            abort_command();
            cam->shared->state = 1;
        }
        if (cam->shared->state == 1) {
            close_command();
            cam->shared->state = 0;
        }
        quit = 1;
    } else if (c == 'r' && strcmp(argv[0], "roi") == 0) {
        int xoff, yoff, width, height;
        CHECK_ARGC(1, 5, 5);
        if (tao_parse_int(argv[1], &xoff) != 0 || xoff < 0 ||
            xoff >= FULLWIDTH) {
            XPAError(xpa, "bad value for `xoff` in set `roi ...` command");
            goto error;
        }
        if (tao_parse_int(argv[2], &yoff) != 0 || yoff < 0 ||
            yoff >= FULLHEIGHT) {
            XPAError(xpa, "bad value for `yoff` in set `roi ...` command");
            goto error;
        }
        if (tao_parse_int(argv[3], &width) != 0 || width < 1 ||
            width > FULLWIDTH) {
            XPAError(xpa, "bad value for `width` in set `roi ...` command");
            goto error;
        }
        if (tao_parse_int(argv[4], &height) != 0 || height < 1 ||
            height > FULLHEIGHT) {
            XPAError(xpa, "bad value for `height` in set `roi ...` command");
            goto error;
        }
        if (xoff + width > FULLWIDTH) {
            XPAError(xpa, "`xoff + width` too large in set `roi ...` "
                     "command");
            goto error;
        }
        if (yoff + height > FULLHEIGHT) {
            XPAError(xpa, "`yoff + height` too large in set `roi ...` "
                     "command");
            goto error;
        }
        if (cam->shared->state >= 2) {
            XPAError(xpa, "cannot change ROI during an acquisition");
            goto error;
        }
        cam->shared->xoff = xoff;
        cam->shared->yoff = yoff;
        cam->shared->width = width;
        cam->shared->height = height;
    } else if (c == 's' && strcmp(argv[0], "start") == 0) {
        int nbufs;
        CHECK_ARGC(1, 1, 2);
        if (argc == 2) {
            if (tao_parse_int(argv[1], &nbufs) != 0 || nbufs <= 0) {
                XPAError(xpa, "invalid number of buffers");
                goto error;
            }
        } else {
            nbufs = 4;
        }
        if (cam->shared->state == 0 || cam->shared->state == 1) {
            start_command(nbufs);
            cam->shared->state = 2;
        } else if (cam->shared->state >= 2) {
            XPAError(xpa, "acquisition is already running");
            goto error;
        }
    } else if (c == 's' && strcmp(argv[0], "stop") == 0) {
        CHECK_ARGC(1, 1, 1);
        if (cam->shared->state == 2) {
            stop_command();
            cam->shared->state = 1;
        } else {
            XPAError(xpa, "no acquisition is running");
            goto error;
        }
    } else {
        XPAError(xpa, "unknown set command or parameter");
        goto error;
    }

 done:
    if (errs != TAO_NO_ERRORS) {
        /* FIXME: This should not happen, at least print a warning. */
        tao_discard_errors(&errs);
    }
    if (argv != NULL) {
        free(argv);
    }
    return status;

 error:
    if (errs != TAO_NO_ERRORS) {
        report_error(&errs, xpa);
    }
    status = -1;
    goto done;
}

#undef CHECK_MIN_ARGC
#undef CHECK_ARGC
#undef INVALID_ARGUMENTS
#undef CMD

/*---------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
    XPA srv;
    char* serverclass = "TAO";
    char* servername = "virtualcamera1";
    char* send_mode = "acl=true,freebuf=false";
    char* recv_mode = "";
    void* send_data = NULL;
    void* recv_data = NULL;
    tao_error_t* errs = NULL;
    int nframes = 10; /* maximum number of frames to memorize */
    int msec = 100; /* milliseconds */

    /* Initialize resources. */
    tao_initialize_static_buffer(&srvbuf);
    cam = tao_create_camera(&errs, nframes, 0660);
    if (cam == NULL) {
        tao_report_errors(&errs);
        exit(1);
    }
    if (debug) {
        fprintf(stderr, "XPA: %s:%s\n", serverclass, servername);
        fprintf(stderr, "shmid: %ld\n",
                (long)tao_get_shared_camera_ident(cam->shared));
    }
    cam->shared->state = 0;
    cam->shared->depth = 8;
    cam->shared->xoff = 0;
    cam->shared->yoff = 0;
    cam->shared->width = 384;
    cam->shared->height = 288;
    cam->shared->fullwidth = FULLWIDTH;
    cam->shared->fullheight = FULLHEIGHT;
    cam->shared->exposure = 0.001;
    cam->shared->rate = 500.0;
    cam->shared->gain = 50.0;
    cam->shared->bias = 500.0;
    cam->shared->gamma = 1.0;
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
    XPACleanup();
    tao_finalize_buffer(&srvbuf);
    if (tao_finalize_camera(&errs, cam) != 0) {
        tao_report_errors(&errs);
    }
    return 0;
}
