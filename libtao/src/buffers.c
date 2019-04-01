/*
 * buffers.c --
 *
 * Implement i/o buffers in TAO library.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include "tao-private.h"

#define if_likely(expr)   if TAO_LIKELY(expr)
#define if_unlikely(expr) if TAO_UNLIKELY(expr)

#define VOLATILE_STRUCT  (1 << 0)
#define VOLATILE_DATA    (1 << 1)

#define GRAIN   64
#define MINSIZE 256

#define CHECK_BUFFER_STRUCT(errs, buf, code)                    \
    do {                                                        \
        if_unlikely(buf == NULL) {                              \
            tao_push_error(errs, __func__, TAO_BAD_ADDRESS);    \
            return code;                                        \
        }                                                       \
        if_unlikely((buf->data == NULL && buf->size != 0) ||    \
            buf->offset > buf->size ||                          \
            buf->pending > buf->size ||                         \
            buf->offset + buf->pending > buf->size) {           \
            tao_push_error(errs, __func__, TAO_CORRUPTED);      \
            return code;                                        \
        }                                                       \
    } while (false)

/* Yields next size of buffer when growing.  If size is less or equal MINSIZE,
   return MINSIZE; otherwise, return 1.5 times the size rounded up to GRAIN
   bytes.  This is to avoid reallocating too many times. */
static size_t
next_buffer_size(size_t size)
{
    if (size <= MINSIZE) {
        return MINSIZE;
    } else {
        return TAO_ROUND_UP(size + (size >> 1), GRAIN);
    }
}

void
tao_initialize_static_buffer(tao_buffer_t* buf)
{
    buf->data = NULL;
    buf->size = 0;
    buf->offset = 0;
    buf->pending = 0;
    buf->flags = 0;
}

tao_buffer_t*
tao_create_buffer(tao_error_t** errs, size_t size)
{
    tao_buffer_t* buf = (tao_buffer_t*)tao_malloc(errs, sizeof(tao_buffer_t));
    if (buf != NULL) {
        buf->data = NULL;
        buf->size = 0;
        buf->offset = 0;
        buf->pending = 0;
        buf->flags = VOLATILE_STRUCT;
        if (size > 0) {
            if (size < MINSIZE) {
                size = MINSIZE;
            } else {
                size = TAO_ROUND_UP(size, GRAIN);
            }
            buf->data = tao_malloc(errs, size);
            if (buf->data == NULL) {
                free((void*)buf);
                return NULL;
            }
            buf->size = size;
            buf->flags |= VOLATILE_DATA;
        }
    }
    return buf;
}

void
tao_finalize_buffer(tao_buffer_t* buf)
{
    if (buf != NULL) {
        void* data = buf->data;
        unsigned int flags = buf->flags;
        if ((flags & VOLATILE_STRUCT) != 0) {
            /* Free the container. */
            free((void*)buf);
        } else {
            /* Reset the container. */
            tao_initialize_static_buffer(buf);
        }
        if (data != NULL && (flags & VOLATILE_DATA) != 0) {
            /* Free the contents. */
            free(data);
        }
    }
}

int
tao_resize_buffer(tao_error_t** errs, tao_buffer_t* buf, size_t cnt)
{
    /* Check arguments. */
    CHECK_BUFFER_STRUCT(errs, buf, -1);

    /* If all contents has been consumed, reset the offset to avoid
       unnecessarily storing bytes in the middle of the buffer. */
    if (buf->pending < 1) {
        buf->offset = 0;
    }

    /* If the number of available bytes in the buffer is insufficient,
       either move the buffer contents or resize the buffer. */
    size_t avail = buf->size - (buf->offset + buf->pending);
    if (cnt > avail) {
        if (buf->pending + cnt <= buf->size) {
            /* Buffer size is sufficient, just move the contents. */
            tao_flush_buffer(buf);
        } else {
            /* Reallocate the buffer. */
            size_t minsize = buf->pending + cnt;
            size_t newsize = next_buffer_size(minsize);
            char* olddata = buf->data;
            char* newdata = (char*)tao_malloc(errs, newsize);
            if (newdata == NULL) {
                return -1;
            }
            if (buf->pending > 0) {
                /* Copy the contents of the old buffer at the beginning of the
                   new buffer. */
                memcpy(newdata, olddata + buf->offset, buf->pending);
            }
            buf->offset = 0;
            buf->data = newdata;
            buf->size = newsize;
            if ((buf->flags & VOLATILE_DATA) == 0) {
                /* Mark the internal data as being volatile. */
                buf->flags |= VOLATILE_DATA;
            } else if (olddata != NULL) {
                /* Free the old volatile data. */
                free((void*)olddata);
            }
        }

        /* Update number of available bytes. */
        avail = buf->size - (buf->offset + buf->pending);
        if (avail < cnt) {
            tao_push_error(errs, __func__, TAO_ASSERTION_FAILED);
            return -1;
        }
    }
    return 0;
}

void
tao_flush_buffer(tao_buffer_t* buf)
{
    if (buf->offset > 0 && buf->pending > 0) {
        /* Move the contents to the beginning of the internal buffer. */
        memmove(buf->data, buf->data + buf->offset, buf->pending);
        buf->offset = 0;
    }
}

void
tao_clear_buffer(tao_buffer_t* buf)
{
    buf->pending = 0;
    buf->offset = 0;
}

size_t
tao_get_buffer_contents_size(tao_buffer_t* buf)
{
    return buf->pending;
}

size_t
tao_get_buffer_contents(const tao_buffer_t* buf, void** data)
{
    *data = buf->data + buf->offset;
    return buf->pending;
}

size_t
tao_get_buffer_unused_size(const tao_buffer_t* buf)
{
    return buf->size - (buf->offset + buf->pending);
}

size_t
tao_get_buffer_total_unused_size(const tao_buffer_t* buf)
{
    return buf->size - buf->pending;
}

size_t
tao_get_buffer_unused_part(const tao_buffer_t* buf, void** data)
{
    /* Offset of first unused byte. */
    size_t offset = buf->offset + buf->pending;
    *data = buf->data + offset;
    return buf->size - offset;
}

int
tao_adjust_buffer_contents_size(tao_error_t** errs, tao_buffer_t* buf,
                                ssize_t adj)
{
    /* Check arguments and make the necessary adjustment. */
    CHECK_BUFFER_STRUCT(errs, buf, -1);
    if (adj < 0) {
        /* Consume some pending bytes. */
        size_t cnt = -adj;
        if (cnt < buf->pending) {
            /* Some contents has been consumed. */
            buf->offset += cnt;
        } else if (cnt == buf->pending) {
            /* All contents has been consumed. */
            buf->offset = 0;
            buf->pending = 0;
        } else {
            /* Refuse to consume more data than available. */
            tao_push_error(errs, __func__, TAO_OUT_OF_RANGE);
            return -1;
        }
    } else if (adj > 0) {
        /* Some data have been appended to the buffer. */
        size_t cnt = +adj;
        size_t maxcnt = buf->size - (buf->offset + buf->pending);
        if (cnt <= maxcnt) {
            /* Grow the contents size. */
            buf->pending += cnt;
        } else {
            /* Refuse to add more data than remaining unused bytes after the
               contents. */
            tao_push_error(errs, __func__, TAO_OUT_OF_RANGE);
            return -1;
        }
    }
    return 0;
}

ssize_t
tao_read_to_buffer(tao_error_t** errs, int fd, tao_buffer_t* buf, size_t cnt)
{
    /* Quick return? */
    if (cnt < 1) {
        return 0;
    }

    /* If all contents has been consumed, reset the offset to avoid
       unnecessarily storing bytes in the middle of the buffer. */
    if (buf->pending < 1) {
        buf->offset = 0;
    }

    /* If the number of available bytes in the buffer is insufficient,
       either move the buffer contents or resize the buffer. */
    size_t avail = buf->size - (buf->offset + buf->pending);
    if (cnt > avail) {
        if (buf->pending + cnt <= buf->size) {
            /* Buffer size is sufficient, just move the contents. */
            tao_flush_buffer(buf);
        } else {
            /* Reallocate the buffer. */
            if (tao_resize_buffer(errs, buf, cnt) == -1) {
                return -1;
            }
        }

        /* Update number of available bytes. */
        avail = buf->size - (buf->offset + buf->pending);
        if (avail < cnt) { /* FIXME: not necessary */
            tao_push_error(errs, __func__, TAO_ASSERTION_FAILED);
            return -1;
        }
    }

    /* Attempt to read as many bytes as possible (that is, not just `cnt`
       bytes).  FIXME: bad idea. */
    ssize_t nr = read(fd, buf->data + buf->offset, avail);
    if (nr == -1) {
        /* Some error occured. */
        tao_push_system_error(errs, "read");
        return -1;
    }
    buf->pending += nr;
    return nr;
}

ssize_t
tao_write_from_buffer(tao_error_t** errs, int fd, tao_buffer_t* buf)
{
    /* Check arguments. */
    CHECK_BUFFER_STRUCT(errs, buf, -1);

    /* If there are no bytes to write.  Reset the offset (flush the contents)
       and return. */
    if (buf->pending < 1) {
        buf->offset = 0;
        return 0;
    }

    /* Attempt to write as many bytes as possible. */
    ssize_t nw = write(fd, buf->data + buf->offset, buf->pending);
    if (nw > 0) {
        if (nw == buf->pending) {
            /* All bytes have been written.  Quickly flush the buffer.  */
            buf->pending = 0;
            buf->offset  = 0;
        } else {
            /* Some bytes have been written. */
            buf->pending -= nw;
            buf->offset  += nw;
        }
        return nw;
    }
    if (nw == -1) {
        int code = errno;
        if (code == EAGAIN || code == EWOULDBLOCK) {
            /* The operation would block. */
            return 0;
        } else {
            /* Some error occured. */
            tao_push_system_error(errs, "read");
            return -1;
        }
    }
    /* No bytes have been written. */
    return 0;
}

int
tao_print_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                    const char* format, ...)
{
    int status;
    va_list args;

    va_start(args, format);
    status = tao_vprint_to_buffer(errs, buf, format, args);
    va_end(args);
    return status;
}

int
tao_vprint_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                     const char* format, va_list args)
{
    CHECK_BUFFER_STRUCT(errs, buf, -1); /* FIXME: optimize */
    long end = buf->offset + buf->pending;
    while (true) {
        /* Append formated message to the i/o buffer, resizing and adjusting
           the size of the buffer as needed. */
        va_list temp;
        long len, siz = buf->size - end;
        va_copy(temp, args);
        len = vsnprintf(buf->data + end, siz, format, temp);
        va_end(temp);
        if (len < siz) {
            if (len < 0) {
                /* Some error occured. */
                tao_push_system_error(errs, "vsnprintf");
                return -1;
            } else {
                /* Unused part was large enough.  Adjust buffer size and
                   return.  The final null byte is not considered as part of
                   the contents. */
                buf->pending += len;
                return 0;
            }
        }
        /* Unused part was too small.  Enlarge it before retrying. */
        if (tao_resize_buffer(errs, buf, len + 1) == -1) {
            return -1;
        }
    }
}

int
tao_append_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                     const void* ptr, long siz)
{
    if (siz > 0) {
        if (tao_resize_buffer(errs, buf, siz) == -1) {
            return -1;
        }
        memcpy(buf->data + buf->offset + buf->pending, ptr, siz);
        buf->pending += siz;
    }
    return 0;
}

int
tao_put_string_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                         const char* str, long len)
{
    if (len < 0) {
        len = (str == NULL || str[0] == '\0' ? 0 : strlen(str));
    }
    if (tao_resize_buffer(errs, buf, len + 1) == -1) {
        return -1;
    }
    long end = buf->offset + buf->pending;
    if (len > 0) {
        memcpy(buf->data + end, str, len);
        buf->pending += len; /* not the terminating null */
    }
    buf->data[end+len] = '\0';
    return 0;
}

int
tao_put_char_to_buffer(tao_error_t** errs, tao_buffer_t* buf, int c)
{
    if (tao_resize_buffer(errs, buf, 2) == -1) {
        return -1;
    }
    long end = buf->offset + buf->pending;
    buf->data[end] = c;
    buf->data[end+1] = '\0';
    buf->pending += 1; /* not the terminating null */
    return 0;
}
