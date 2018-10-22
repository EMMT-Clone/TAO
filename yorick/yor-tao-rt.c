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

static void push_string(const char* str);

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
        int ident = tao_get_shared_object_ident(obj);
        int type = tao_get_shared_object_type(obj);
        size_t size = tao_get_shared_object_size(obj);
        sprintf(buffer, "TAO shared object (ident=%d, type=%d, size=%ld)",
                ident, type, (long)size);
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
       we only read immutable information. */
    tao_shared_object_t* obj = *(tao_shared_object_t**)addr;
    if (strcmp(name, "ident") == 0) {
        ypush_int(tao_get_shared_object_ident(obj));
    } else if (strcmp(name, "type") == 0) {
        ypush_int(tao_get_shared_object_type(obj));
    } else if (strcmp(name, "size") == 0) {
        ypush_long(tao_get_shared_object_size(obj));
    } else {
        y_error("bad shared object member");
    }
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

#if 0
static tao_shared_array_t* get_shared_array(int iarg)
{
    tao_shared_object_t* obj = get_shared_object(iarg);
    if (tao_get_shared_object_type(obj) != TAO_SHARED_ARRAY) {
        y_error("invalid TAO object type");
    }
    return (tao_shared_array_t*)obj;
}

static tao_shared_camera_t* get_shared_camera(int iarg)
{
    tao_shared_object_t* obj = get_shared_object(iarg);
    if (tao_get_shared_object_type(obj) != TAO_SHARED_ARRAY) {
        y_error("invalid TAO object type");
    }
    return (tao_shared_camera_t*)obj;
}
#endif

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
    tao_error_t* errs = TAO_NO_ERRORS;
    int code;
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    code = tao_lock_shared_object(&errs, get_shared_object(0));
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    ypush_int(code == 0);
}

void
Y_tao_unlock(int argc)
{
    tao_error_t* errs = TAO_NO_ERRORS;
    int code;
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    code = tao_unlock_shared_object(&errs, get_shared_object(0));
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    ypush_int(code == 0);
}

void
Y_tao_try_lock(int argc)
{
    tao_error_t* errs = TAO_NO_ERRORS;
    int code;
    if (argc != 1) {
        y_error("expecting exactly one argument");
    }
    code = tao_try_lock_shared_object(&errs, get_shared_object(0));
    if (errs != TAO_NO_ERRORS) {
        report_errors(&errs);
    }
    ypush_int(code);
}

void
Y_tao_test(int argc)
{
    push_string("hello!");
}

static void
push_string(const char* str)
{
  ypush_q(NULL)[0] = p_strcpy(str);
}

void
Y__tao_init(int argc)
{
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
#undef DEF_LONG_CONST
    ypush_nil();
}

/*---------------------------------------------------------------------------*/
