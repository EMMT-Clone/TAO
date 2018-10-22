/*
 * yor-tao-rt.c --
 *
 * Implements Yorick interface to TAO real-time software.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 2018: Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>
 *
 * See LICENSE.md for details.
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include <pstdlib.h>
#include <play.h>
#include <yapi.h>

#include <tao.h>

/* Define some macros to get rid of some GNU extensions when not compiling
   with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#   define __attribute__(x)
#   define __inline__
#   define __FUNCTION__        ""
#   define __PRETTY_FUNCTION__ ""
#endif

PLUG_API void y_error(const char *) __attribute__ ((noreturn));

static int tao_to_yorick_types[TAO_FLOAT64 - TAO_INT8 + 1];
static int yorick_to_tao_types[Y_DOUBLE - Y_CHAR + 1];
static char* buffer = NULL;
static size_t bufsiz = 0;

static void resize_buffer(size_t size)
{
    size_t minsize = size;
    size_t maxsize = size + ((size + 1)>>1);
    if (buffer == NULL || bufsiz < minsize || bufsiz > maxsize) {
        void* oldbuf = buffer;
        void* newbuf = p_malloc(maxsize);
        if (oldbuf != NULL && bufsiz > 0) {
            memcpy(newbuf, oldbuf, (maxsize < bufsiz ? maxsize : bufsiz));
        }
        buffer = newbuf;
        bufsiz = maxsize;
        if (oldbuf != NULL) {
            p_free(oldbuf);
        }
    }
}

static void
report_errors(tao_error_t** errs)
{
    const char* func;
    long buflen;
    int code;

    if (bufsiz < 100) {
        resize_buffer(100);
    }
    buflen = 0;
    while (tao_pop_error(errs, &func, &code)) {
        const char* reason = tao_get_error_reason(code);
        const char* errnam = tao_get_error_name(code);
        while (1) {
            long len = snprintf(buffer + buflen, bufsiz - buflen,
                                "%s in function `%s` [%s]\n",
                                reason, func, errnam);
            if (buflen + len < bufsiz) {
                buflen += len;
                break;
            }
            resize_buffer(buflen + len + 1);
        }
    }
    y_error(buffer);
}

static void push_array_data(tao_shared_array_t* arr)
{
    long dims[Y_DIMSIZE];
    int d, ndims, type;
    const void* src;
    void* dst;
    long nelem, elsize;

    type = tao_get_shared_array_eltype(arr);
    if (type >= TAO_INT8 && type <= TAO_FLOAT64) {
        type = tao_to_yorick_types[type - TAO_INT8];
    } else {
        type = -1;
    }
    ndims = tao_get_shared_array_ndims(arr);
    if (ndims < 0) {
        y_error("invalid number of dimensions");
    }
    if (ndims >= Y_DIMSIZE) {
        y_error("too many dimensions");
    }
    dims[0] = ndims;
    nelem = 1;
    for (d = 1; d <= ndims; ++d) {
        dims[d] = tao_get_shared_array_size(arr, d);
        nelem *= dims[d];
    }
    switch (type) {
    case Y_CHAR:
        elsize = sizeof(char);
        dst = ypush_c(dims);
        break;
    case Y_SHORT:
        elsize = sizeof(short);
        dst = ypush_s(dims);
        break;
    case Y_INT:
        elsize = sizeof(int);
        dst = ypush_i(dims);
        break;
    case Y_LONG:
        elsize = sizeof(long);
        dst = ypush_l(dims);
        break;
    case Y_FLOAT:
        elsize = sizeof(float);
        dst = ypush_f(dims);
        break;
    case Y_DOUBLE:
        elsize = sizeof(double);
        dst = ypush_d(dims);
        break;
    default:
        y_error("unsupported array type");
        elsize = 0;
        dst = NULL;
    }
    src = tao_get_shared_array_data(arr);
    if (elsize > 0 && nelem > 0) {
        memcpy(dst, src, nelem*elsize);
    }
}

static void push_array_timestamp(tao_shared_array_t* arr)
{
    int64_t ts_sec, ts_nsec;
    long dims[2];
    long* ans;
    tao_get_shared_array_timestamp(arr, &ts_sec, &ts_nsec);
    dims[0] = 1;
    dims[1] = 2;
    ans = ypush_l(dims);
    ans[0] = ts_sec;
    ans[1] = ts_nsec;
}

/*---------------------------------------------------------------------------*/
/* SHARED OBJECTS */

static void
free_shared_object(void* addr)
{
    tao_shared_object_t* obj = *(tao_shared_object_t**)addr;
    tao_error_t* errs = TAO_NO_ERRORS;
    tao_detach_shared_object(&errs, obj);
    if (errs != TAO_NO_ERRORS) {
        /* At this point, we can just print the errors if nay. */
        tao_report_errors(&errs);
    }
}

static void
print_shared_object(void* addr)
{
    tao_shared_object_t* obj = *(tao_shared_object_t**)addr;
    tao_error_t* errs = TAO_NO_ERRORS;
    if (bufsiz < 200) {
        resize_buffer(200);
    }
    if (tao_lock_shared_object(&errs, obj) == 0) {
        char small_buf[64];
        char* type_name;
        int ident = tao_get_shared_object_ident(obj);
        int type = tao_get_shared_object_type(obj);
        size_t size = tao_get_shared_object_size(obj);
        if (type == TAO_SHARED_ARRAY) {
            type_name = "TAO_SHARED_ARRAY";
        } else if (type == TAO_SHARED_CAMERA) {
            type_name = "TAO_SHARED_CAMERA";
        } else if (type == TAO_SHARED_OBJECT) {
            type_name = "TAO_SHARED_OBJECT";
        } else {
            sprintf(small_buf, "0x%08x", (unsigned int)type);
            type_name = small_buf;
        }
        sprintf(buffer, "TAO shared object (ident=%d, type=%s, size=%ld)",
                ident, type_name, (long)size);
        tao_unlock_shared_object(&errs, obj);
    } else {
        buffer[0] = '\0';
    }
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    y_print(buffer, 1);
}

static void
eval_shared_object(void* addr, int argc)
{
    tao_shared_object_t* obj = *(tao_shared_object_t**)addr;
    tao_error_t* errs = TAO_NO_ERRORS;
    int ident = -1;
    if (tao_lock_shared_object(&errs, obj) == 0) {
        ident = tao_get_shared_object_ident(obj);
        tao_unlock_shared_object(&errs, obj);
    }
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    ypush_int(ident);
}

static void
extract_shared_object(void* addr, char* name)
{
    /* There is no needs to lock a shared object if we already have a reference
       on it (which is warranted if it is attached to our address space) and if
       we only read immutable information.  Using string comparison is fast
       enough (worst case here takes 75 ns, best case takes 58 ns). */
    tao_shared_object_t* obj = *(tao_shared_object_t**)addr;
    int c = name[0];
    if (c == 'i' && strcmp(name, "ident") == 0) {
        ypush_int(tao_get_shared_object_ident(obj));
        return;
    } else if (c == 't' && strcmp(name, "type") == 0) {
        ypush_int(tao_get_shared_object_type(obj));
        return;
    } else if (c == 's' && strcmp(name, "size") == 0) {
        ypush_long(tao_get_shared_object_size(obj));
        return;
    } else {
        int type = tao_get_shared_object_type(obj);
        if (type == TAO_SHARED_ARRAY) {
            tao_shared_array_t* arr = (tao_shared_array_t*)obj;
            if (c == 'c' && strcmp(name, "counter") == 0) {
                ypush_long(tao_get_shared_array_counter(arr));
                return;
            } else if (c == 'e' && strcmp(name, "eltype") == 0) {
                ypush_long(tao_get_shared_array_eltype(arr));
                return;
            } else if (c == 'd' && strcmp(name, "data") == 0) {
                push_array_data(arr);
                return;
            } else if (c == 'd' && strcmp(name, "dims") == 0) {
                long dimsofdims[2];
                long* dims;
                int d, ndims;
                ndims = tao_get_shared_array_ndims(arr);
                if (ndims < 0) {
                    y_error("invalid number of dimensions");
                }
                dimsofdims[0] = 1;
                dimsofdims[1] = ndims + 1;
                dims = ypush_l(dimsofdims);
                dims[0] = ndims;
                for (d = 1; d <= ndims; ++d) {
                    dims[d] = tao_get_shared_array_size(arr, d);
                }
                return;
            } else if (c == 'n' && strcmp(name, "ndims") == 0) {
                ypush_long(tao_get_shared_array_ndims(arr));
                return;
            } else if (c == 'n' && strcmp(name, "nreaders") == 0) {
                ypush_long(tao_get_shared_array_nreaders(arr));
                return;
            } else if (c == 'n' && strcmp(name, "nwriters") == 0) {
                ypush_long(tao_get_shared_array_nwriters(arr));
                return;
            } else if (c == 't' && strcmp(name, "timestamp") == 0) {
                push_array_timestamp(arr);
                return;
            }
        } else if (type == TAO_SHARED_CAMERA) {
            tao_shared_camera_t* cam = (tao_shared_camera_t*)obj;
            if (c == 'b' && strcmp(name, "bias") == 0) {
                ypush_double(tao_get_shared_camera_bias(cam));
                return;
            } else if (c == 'd' && strcmp(name, "depth") == 0) {
                ypush_long(tao_get_shared_camera_depth(cam));
                return;
            } else if (c == 'e' && strcmp(name, "eltype") == 0) {
                ypush_long(tao_get_shared_camera_pixel_type(cam));
                return;
            } else if (c == 'e' && strcmp(name, "exposure") == 0) {
                ypush_double(tao_get_shared_camera_exposure(cam));
                return;
            } else if (c == 'f' && strcmp(name, "fullheight") == 0) {
                ypush_long(tao_get_shared_camera_fullheight(cam));
                return;
            } else if (c == 'f' && strcmp(name, "fullwidth") == 0) {
                ypush_long(tao_get_shared_camera_fullwidth(cam));
                return;
            } else if (c == 'g' && strcmp(name, "gain") == 0) {
                ypush_double(tao_get_shared_camera_gain(cam));
                return;
            } else if (c == 'g' && strcmp(name, "gamma") == 0) {
                ypush_double(tao_get_shared_camera_gamma(cam));
                return;
            } else if (c == 'h' && strcmp(name, "height") == 0) {
                ypush_long(tao_get_shared_camera_height(cam));
                return;
            } else if (c == 'r' && strcmp(name, "roi") == 0) {
                long dims[2];
                long* roi;
                dims[0] = 1;
                dims[1] = 4;
                roi = ypush_l(dims);
                roi[0] = tao_get_shared_camera_xoff(cam);
                roi[1] = tao_get_shared_camera_yoff(cam);
                roi[2] = tao_get_shared_camera_width(cam);
                roi[3] = tao_get_shared_camera_height(cam);
                return;
            } else if (c == 'r' && strcmp(name, "rate") == 0) {
                ypush_double(tao_get_shared_camera_rate(cam));
                return;
            } else if (c == 's' && strcmp(name, "state") == 0) {
                ypush_int(tao_get_shared_camera_state(cam));
                return;
            } else if (c == 'w' && strcmp(name, "width") == 0) {
                ypush_long(tao_get_shared_camera_width(cam));
                return;
            } else if (c == 'x' && strcmp(name, "xoff") == 0) {
                ypush_long(tao_get_shared_camera_xoff(cam));
                return;
            } else if (c == 'y' && strcmp(name, "yoff") == 0) {
                ypush_long(tao_get_shared_camera_yoff(cam));
                return;
            }
        }
    }
    y_error("bad member");
}

static y_userobj_t shared_object_type = {
    "tao_shared_object",
    free_shared_object,
    print_shared_object,
    eval_shared_object,
    extract_shared_object,
    NULL
};

static tao_shared_object_t* get_shared_object(int iarg)
{
    return *(tao_shared_object_t**)yget_obj(iarg, &shared_object_type);
}

static tao_shared_array_t* get_shared_array(int iarg)
{
    tao_shared_object_t* obj = get_shared_object(iarg);
    if (tao_get_shared_object_type(obj) != TAO_SHARED_ARRAY) {
        y_error("TAO shared object is not an array");
    }
    return (tao_shared_array_t*)obj;
}

static tao_shared_camera_t* get_shared_camera(int iarg)
{
    tao_shared_object_t* obj = get_shared_object(iarg);
    if (tao_get_shared_object_type(obj) != TAO_SHARED_CAMERA) {
        y_error("TAO shared object is not a camera");
    }
    return (tao_shared_camera_t*)obj;
}

void
Y_tao_attach_shared_object(int argc)
{
    tao_shared_object_t* obj;
    tao_error_t* errs = TAO_NO_ERRORS;
    int ident, type;
    if (argc < 1 || argc > 2) {
        y_error("expecting 1 or 2 arguments");
    }
    ident = ygets_l(argc - 1);
    type = (argc >= 2 ? ygets_l(argc - 2) : TAO_SHARED_ANY);
    if (0x00 <= type && type <= 0xff) {
        type |= TAO_SHARED_MAGIC;
    }
    obj = tao_attach_shared_object(&errs, ident, type);
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    *(void**)ypush_obj(&shared_object_type, sizeof(void*)) = obj;
}

void
Y_tao_attach_shared_array(int argc)
{
    tao_shared_array_t* arr;
    tao_error_t* errs = TAO_NO_ERRORS;
    int ident, type;
    if (argc != 1) {
        y_error("expecting exactly 1 argument");
    }
    ident = ygets_l(argc - 1);
    type = (argc >= 2 ? ygets_l(argc - 2) : TAO_SHARED_ANY);
    if (0x00 <= type && type <= 0xff) {
        type |= TAO_SHARED_MAGIC;
    }
    arr = tao_attach_shared_array(&errs, ident);
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    *(void**)ypush_obj(&shared_object_type, sizeof(void*)) = arr;
}

void
Y_tao_attach_shared_camera(int argc)
{
    tao_shared_camera_t* cam;
    tao_error_t* errs = TAO_NO_ERRORS;
    int ident, type;
    if (argc != 1) {
        y_error("expecting exactly 1 argument");
    }
    ident = ygets_l(argc - 1);
    type = (argc >= 2 ? ygets_l(argc - 2) : TAO_SHARED_ANY);
    if (0x00 <= type && type <= 0xff) {
        type |= TAO_SHARED_MAGIC;
    }
    cam = tao_attach_shared_camera(&errs, ident);
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    *(void**)ypush_obj(&shared_object_type, sizeof(void*)) = cam;
}

void
Y_tao_create_shared_object(int argc)
{
    tao_shared_object_t* obj;
    tao_error_t* errs = TAO_NO_ERRORS;
    unsigned int perms;
    unsigned int type;
    long size;
    if (argc < 1 || argc > 3) {
        y_error("expecting 1 to 3 arguments");
    }
    type = ygets_l(argc - 1);
    size = (argc >= 2 ? ygets_l(argc - 2) : TAO_SHARED_MIN_SIZE);
    perms = (argc >= 3 ? ygets_l(argc - 3) : 0);
    if (0x00 <= type && type <= 0xff) {
        type |= TAO_SHARED_MAGIC;
    }
    if (size < TAO_SHARED_MIN_SIZE) {
        y_error("invalid size");
    }
    obj = tao_create_shared_object(&errs, type, size, perms);
    if (obj == NULL) {
        report_errors(&errs);
    } else {
        tao_discard_errors(&errs);
        *(void**)ypush_obj(&shared_object_type, sizeof(void*)) = obj;
    }
}

void
Y_tao_wait_image(int argc)
{
    tao_shared_camera_t* cam;
    tao_error_t* errs = TAO_NO_ERRORS;
    int ans, ident, iarg, sem;

    /* Check number of arguments. */
    if (argc < 2 || argc > 3) {
        y_error("expecting 2 or 3 arguments");
    }

    /* Parse first argument. */
    iarg = argc - 1;
    cam = get_shared_camera(iarg);

    /* Parse second argument. */
    iarg = argc - 2;
    sem = ygets_l(iarg);
    if (sem <= 0) {
        /* Apply Yorick indexing rules. */
        sem += TAO_SHARED_CAMERA_SEMAPHORES;
    }
    if (sem < 1 || sem > TAO_SHARED_CAMERA_SEMAPHORES) {
        y_error("out of range semaphore number");
    }

    /* Parse third (optional) argument and wait for next image. */
    iarg = argc - 3;
    if (iarg < 0 || yarg_nil(iarg)) {
        if (tao_wait_image(&errs, cam, sem) == 0) {
            ans = 1;
        } else {
            ans = -1;
        }
    } else {
        double secs = ygets_d(iarg);
        ans = tao_timed_wait_image(&errs, cam, sem, secs);
    }
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }

    /* Attach new image if any. */
    if (ans == 1) {
        ident = tao_get_last_image_ident(cam);
    } else {
        ident = -1;
    }
    if (ident == -1) {
        ypush_nil();
    } else {
        tao_shared_array_t* arr = tao_attach_shared_array(&errs, ident);
        if (errs != TAO_NO_ERRORS) {
            report_errors(&errs);
        }
        *(void**)ypush_obj(&shared_object_type, sizeof(void*)) = arr;
    }
}

void
Y_tao_get_current_time(int argc)
{
    long dims[2];
    long* ptr;
    tao_time_t t;
    tao_error_t* errs = TAO_NO_ERRORS;
    tao_get_current_time(&errs, &t);
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    dims[0] = 1;
    dims[1] = 2;
    ptr = ypush_l(dims);
    ptr[0] = t.s;
    ptr[1] = t.ns;
}

void
Y_tao_get_monotonic_time(int argc)
{
    long dims[2];
    long* ptr;
    tao_time_t t;
    tao_error_t* errs = TAO_NO_ERRORS;
    tao_get_monotonic_time(&errs, &t);
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    dims[0] = 1;
    dims[1] = 2;
    ptr = ypush_l(dims);
    ptr[0] = t.s;
    ptr[1] = t.ns;
}

/* lock + unlock ~ 0.14 µs */
void
Y_tao_lock(int argc)
{
    tao_error_t* errs;
    tao_shared_object_t* obj;
    int code;

    /* Check arguments. */
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    obj = get_shared_object(0);

    /* Check that there are no pending signals before locking. */
    if (p_signalling) {
        p_abort();
    }

    /* Lock object. */
    errs = TAO_NO_ERRORS;
    code = tao_lock_shared_object(&errs, obj);
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    ypush_int(code == 0);
}

void
Y_tao_unlock(int argc)
{
    tao_error_t* errs;
    tao_shared_object_t* obj;
    int code;

    /* Check arguments. */
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    obj = get_shared_object(0);

    /* Unlock object. */
    errs = TAO_NO_ERRORS;
    code = tao_unlock_shared_object(&errs, obj);
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    ypush_int(code == 0);
}

void
Y_tao_try_lock(int argc)
{
    tao_error_t* errs;
    tao_shared_object_t* obj;
    int code;

    /* Check arguments. */
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    obj = get_shared_object(0);

    /* Check that there are no pending signals before locking. */
    if (p_signalling) {
        p_abort();
    }

    /* Try to lock object. */
    errs = TAO_NO_ERRORS;
    code = tao_try_lock_shared_object(&errs, obj);
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    ypush_int(code);
}

void
Y_tao_get_data(int argc)
{
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    push_array_data(get_shared_array(0));
}

/*
 * Calling a built-in function is slightly faster than extracting an object
 * member:
 *
 *  `tao_get_counter(arr)` -----> 32 ns     `arr.counter` -----> 56 ns
 *  `tao_get_timestamp(arr)` ---> 76 ns     `arr.timestamp` ---> 106 ns
 *
 * So it seems that there is an overhead of ~ 30 ns when extracting an object
 * member.
 */
void
Y_tao_get_counter(int argc)
{
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    ypush_long(tao_get_shared_array_counter(get_shared_array(0)));
}

void
Y_tao_get_timestamp(int argc)
{
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    push_array_timestamp(get_shared_array(0));
}

static void initialize_type(size_t size, int ytype, int floatingpoint)
{
    if (floatingpoint) {
        if (size == 4) {
            tao_to_yorick_types[TAO_FLOAT32 - TAO_INT8] = ytype;
            yorick_to_tao_types[ytype - Y_CHAR] = TAO_FLOAT32;
        } else if (size == 8) {
            tao_to_yorick_types[TAO_FLOAT64 - TAO_INT8] = ytype;
            yorick_to_tao_types[ytype - Y_CHAR] = TAO_FLOAT64;
        }
    } else {
        if (size == 1) {
            tao_to_yorick_types[TAO_INT8 - TAO_INT8] = ytype;
            tao_to_yorick_types[TAO_UINT8 - TAO_INT8] = ytype;
            yorick_to_tao_types[ytype - Y_CHAR] = TAO_UINT8;
        } else if (size == 2) {
            tao_to_yorick_types[TAO_INT16 - TAO_INT8] = ytype;
            tao_to_yorick_types[TAO_UINT16 - TAO_INT8] = ytype;
            yorick_to_tao_types[ytype - Y_CHAR] = TAO_INT16;
        } else if (size == 4) {
            tao_to_yorick_types[TAO_INT32 - TAO_INT8] = ytype;
            tao_to_yorick_types[TAO_UINT32 - TAO_INT8] = ytype;
            yorick_to_tao_types[ytype - Y_CHAR] = TAO_UINT32;
        } else if (size == 8) {
            tao_to_yorick_types[TAO_INT64 - TAO_INT8] = ytype;
            tao_to_yorick_types[TAO_UINT64 - TAO_INT8] = ytype;
            yorick_to_tao_types[ytype - Y_CHAR] = TAO_UINT64;
        }
    }
}

void
Y__tao_init(int argc)
{
    int i;
    for (i = TAO_INT8; i <= TAO_FLOAT64; ++i) {
        tao_to_yorick_types[i - TAO_INT8] = -1;
    }
    for (i = Y_CHAR; i <= Y_DOUBLE; ++i) {
        yorick_to_tao_types[i - Y_CHAR] = -1;
    }
    initialize_type(sizeof(char),   Y_CHAR,   0);
    initialize_type(sizeof(short),  Y_SHORT,  0);
    initialize_type(sizeof(int),    Y_INT,    0);
    initialize_type(sizeof(long),   Y_LONG,   0);
    initialize_type(sizeof(float),  Y_FLOAT,  1);
    initialize_type(sizeof(double), Y_DOUBLE, 1);

#define DEF_LONG_CONST(id)                      \
    do {                                        \
        ypush_long(id);                         \
        yput_global(yfind_global(#id, 0), 0);   \
        yarg_drop(1);                           \
    } while (0)
    DEF_LONG_CONST(TAO_SHARED_MAGIC);
    DEF_LONG_CONST(TAO_SHARED_OBJECT);
    DEF_LONG_CONST(TAO_SHARED_ARRAY);
    DEF_LONG_CONST(TAO_SHARED_CAMERA);
    DEF_LONG_CONST(TAO_SHARED_CAMERA_SEMAPHORES);
    DEF_LONG_CONST(TAO_INT8);
    DEF_LONG_CONST(TAO_UINT8);
    DEF_LONG_CONST(TAO_INT16);
    DEF_LONG_CONST(TAO_UINT16);
    DEF_LONG_CONST(TAO_INT32);
    DEF_LONG_CONST(TAO_UINT32);
    DEF_LONG_CONST(TAO_INT64);
    DEF_LONG_CONST(TAO_UINT64);
    DEF_LONG_CONST(TAO_FLOAT32);
    DEF_LONG_CONST(TAO_FLOAT64);
#undef DEF_LONG_CONST
    ypush_nil();
}

/*---------------------------------------------------------------------------*/
