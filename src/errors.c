/*
 * errors.c --
 *
 * Management of errors in TAO library.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "tao-private.h"

static const char* blank_prefix = "     - ";
static const char* error_prefix = "{ERROR}";
static const char* fatal_prefix = "{FATAL}";

/* Private structure used to store error information. */
struct tao_error {
    const char*  func; /**< Name of function where error occured */
    int          code; /**< Numerical identifier of the error */
    tao_error_t* prev; /**< Previous error */
};

static void
report_error(const char* prefix, const char* func, int code)
{
    fprintf(stderr, "%s %s in function `%s` [%s]", prefix,
            tao_get_error_text(code), func, tao_get_error_id(code));
}

static void
report_errors(const char* prefix, tao_error_t** errs)
{
    if (errs != NULL) {
        tao_error_t* err;
        while ((err = *errs) != TAO_NO_ERRORS) {
            *errs = err->prev;
            report_error(prefix, err->func, err->code);
            free((void*)err);
            prefix = blank_prefix;
        }
    }
}

static void
panic(const char* func, int code)
{
    if (func != NULL) {
        report_error(fatal_prefix, func, code);
    }
    abort();
}

void
tao_push_error(tao_error_t** errs, const char* func, int code)
{
    if (errs == NULL) {
        /* There is no means to track errors.  Immediately report the error and
           abort the process. */
        panic(func, code);
    } else {
        /* Insert a new error in the list of tracked errors. */
        tao_error_t* err = (tao_error_t*)malloc(sizeof(tao_error_t));
        if (err == NULL) {
            /* Insufficient memory for tracking the error.  Report all errors
               so far including the last one and panic. */
            report_error(fatal_prefix, __func__, TAO_CANT_TRACK_ERROR);
            report_error(blank_prefix, func, code);
            report_errors(blank_prefix, errs);
            panic(NULL, 0);
        } else {
            /* Instanciate error information and insert it in the chained list
               of tracked errors. */
            err->func = func;
            err->code = code;
            err->prev = *errs;
            *errs = err;
        }
    }
}

void
tao_push_system_error(tao_error_t** errs, const char* func)
{
    int code = errno;
    tao_push_error(errs, func, (code > 0 ? code : TAO_SYSTEM_ERROR));
}

void
tao_report_errors(tao_error_t** errs)
{
    report_errors(error_prefix, errs);
}

void
tao_discard_errors(tao_error_t** errs)
{
    if (errs != NULL) {
        tao_error_t* err;
        while ((err = *errs) != TAO_NO_ERRORS) {
            *errs = err->prev;
            free((void*)err);
        }
    }
}

const char*
tao_get_error_text(int code)
{
    if (code > 0) {
        return strerror(code);
    }
    switch (code) {
    case TAO_SUCCESS:          return "Operation was successful";
    case TAO_BAD_MAGIC:        return "Invalid magic number";
    case TAO_BAD_SERIAL:       return "Invalid serial number";
    case TAO_BAD_SIZE:         return "Invalid size";
    case TAO_BAD_TYPE:         return "Invalid type";
    case TAO_BAD_RANK:         return "Invalid number of dimensions";
    case TAO_DESTROYED:        return "Ressource has been destroyed";
    case TAO_SYSTEM_ERROR:     return "Unknown system error";
    case TAO_CANT_TRACK_ERROR: return "Insufficient memory for tracking error";
    default:                   return "Unknown error";
    }
}

const char*
tao_get_error_id(int code)
{
#define CASE(ID) case ID: return #ID
    switch (code) {
        CASE(TAO_SUCCESS);
        CASE(TAO_BAD_MAGIC);
        CASE(TAO_BAD_SERIAL);
        CASE(TAO_BAD_SIZE);
        CASE(TAO_BAD_TYPE);
        CASE(TAO_BAD_RANK);
        CASE(TAO_DESTROYED);
        CASE(TAO_SYSTEM_ERROR);
        CASE(TAO_CANT_TRACK_ERROR);
#if defined(E2BIG) && (!defined(EOVERFLOW) || (E2BIG != EOVERFLOW))
        CASE(E2BIG);
#endif
#ifdef EACCES
        CASE(EACCES);
#endif
#ifdef EADDRINUSE
        CASE(EADDRINUSE);
#endif
#ifdef EADDRNOTAVAIL
        CASE(EADDRNOTAVAIL);
#endif
#ifdef EADV
        CASE(EADV);
#endif
#ifdef EAFNOSUPPORT
        CASE(EAFNOSUPPORT);
#endif
#ifdef EAGAIN
        CASE(EAGAIN);
#endif
#ifdef EALIGN
        CASE(EALIGN);
#endif
#if defined(EALREADY) && (!defined(EBUSY) || (EALREADY != EBUSY))
        CASE(EALREADY);
#endif
#ifdef EBADE
        CASE(EBADE);
#endif
#ifdef EBADF
        CASE(EBADF);
#endif
#ifdef EBADFD
        CASE(EBADFD);
#endif
#ifdef EBADMSG
        CASE(EBADMSG);
#endif
#ifdef ECANCELED
        CASE(ECANCELED);
#endif
#ifdef EBADR
        CASE(EBADR);
#endif
#ifdef EBADRPC
        CASE(EBADRPC);
#endif
#ifdef EBADRQC
        CASE(EBADRQC);
#endif
#ifdef EBADSLT
        CASE(EBADSLT);
#endif
#ifdef EBFONT
        CASE(EBFONT);
#endif
#ifdef EBUSY
        CASE(EBUSY);
#endif
#ifdef ECHILD
        CASE(ECHILD);
#endif
#ifdef ECHRNG
        CASE(ECHRNG);
#endif
#ifdef ECOMM
        CASE(ECOMM);
#endif
#ifdef ECONNABORTED
        CASE(ECONNABORTED);
#endif
#ifdef ECONNREFUSED
        CASE(ECONNREFUSED);
#endif
#ifdef ECONNRESET
        CASE(ECONNRESET);
#endif
#if defined(EDEADLK) && (!defined(EWOULDBLOCK) || (EDEADLK != EWOULDBLOCK))
        CASE(EDEADLK);
#endif
#if defined(EDEADLOCK) && (!defined(EDEADLK) || (EDEADLOCK != EDEADLK))
        CASE(EDEADLOCK);
#endif
#ifdef EDESTADDRREQ
        CASE(EDESTADDRREQ);
#endif
#ifdef EDIRTY
        CASE(EDIRTY);
#endif
#ifdef EDOM
        CASE(EDOM);
#endif
#ifdef EDOTDOT
        CASE(EDOTDOT);
#endif
#ifdef EDQUOT
        CASE(EDQUOT);
#endif
#ifdef EDUPPKG
        CASE(EDUPPKG);
#endif
#ifdef EEXIST
        CASE(EEXIST);
#endif
#ifdef EFAULT
        CASE(EFAULT);
#endif
#ifdef EFBIG
        CASE(EFBIG);
#endif
#ifdef EHOSTDOWN
        CASE(EHOSTDOWN);
#endif
#ifdef EHOSTUNREACH
        CASE(EHOSTUNREACH);
#endif
#if defined(EIDRM) && (!defined(EINPROGRESS) || (EIDRM != EINPROGRESS))
        CASE(EIDRM);
#endif
#ifdef EINIT
        CASE(EINIT);
#endif
#ifdef EINPROGRESS
        CASE(EINPROGRESS);
#endif
#ifdef EINTR
        CASE(EINTR);
#endif
#ifdef EINVAL
        CASE(EINVAL);
#endif
#ifdef EIO
        CASE(EIO);
#endif
#ifdef EISCONN
        CASE(EISCONN);
#endif
#ifdef EISDIR
        CASE(EISDIR);
#endif
#ifdef EISNAME
        CASE(EISNAM);
#endif
#ifdef ELBIN
        CASE(ELBIN);
#endif
#ifdef EL2HLT
        CASE(EL2HLT);
#endif
#ifdef EL2NSYNC
        CASE(EL2NSYNC);
#endif
#ifdef EL3HLT
        CASE(EL3HLT);
#endif
#ifdef EL3RST
        CASE(EL3RST);
#endif
#ifdef ELIBACC
        CASE(ELIBACC);
#endif
#ifdef ELIBBAD
        CASE(ELIBBAD);
#endif
#ifdef ELIBEXEC
        CASE(ELIBEXEC);
#endif
#if defined(ELIBMAX) && (!defined(ECANCELED) || (ELIBMAX != ECANCELED))
        CASE(ELIBMAX);
#endif
#ifdef ELIBSCN
        CASE(ELIBSCN);
#endif
#ifdef ELNRNG
        CASE(ELNRNG);
#endif
#if defined(ELOOP) && (!defined(ENOENT) || (ELOOP != ENOENT))
        CASE(ELOOP);
#endif
#ifdef EMFILE
        CASE(EMFILE);
#endif
#ifdef EMLINK
        CASE(EMLINK);
#endif
#ifdef EMSGSIZE
        CASE(EMSGSIZE);
#endif
#ifdef EMULTIHOP
        CASE(EMULTIHOP);
#endif
#ifdef ENAMETOOLONG
        CASE(ENAMETOOLONG);
#endif
#ifdef ENAVAIL
        CASE(ENAVAIL);
#endif
#ifdef ENET
        CASE(ENET);
#endif
#ifdef ENETDOWN
        CASE(ENETDOWN);
#endif
#ifdef ENETRESET
        CASE(ENETRESET);
#endif
#ifdef ENETUNREACH
        CASE(ENETUNREACH);
#endif
#ifdef ENFILE
        CASE(ENFILE);
#endif
#ifdef ENOANO
        CASE(ENOANO);
#endif
#if defined(ENOBUFS) && (!defined(ENOSR) || (ENOBUFS != ENOSR))
        CASE(ENOBUFS);
#endif
#ifdef ENOCSI
        CASE(ENOCSI);
#endif
#if defined(ENODATA) && (!defined(ECONNREFUSED) || (ENODATA != ECONNREFUSED))
        CASE(ENODATA);
#endif
#ifdef ENODEV
        CASE(ENODEV);
#endif
#ifdef ENOENT
        CASE(ENOENT);
#endif
#ifdef ENOEXEC
        CASE(ENOEXEC);
#endif
#ifdef ENOLCK
        CASE(ENOLCK);
#endif
#ifdef ENOLINK
        CASE(ENOLINK);
#endif
#ifdef ENOMEM
        CASE(ENOMEM);
#endif
#ifdef ENOMSG
        CASE(ENOMSG);
#endif
#ifdef ENONET
        CASE(ENONET);
#endif
#ifdef ENOPKG
        CASE(ENOPKG);
#endif
#ifdef ENOPROTOOPT
        CASE(ENOPROTOOPT);
#endif
#ifdef ENOSPC
        CASE(ENOSPC);
#endif
#if defined(ENOSR) && (!defined(ENAMETOOLONG) || (ENAMETOOLONG != ENOSR))
        CASE(ENOSR);
#endif
#if defined(ENOSTR) && (!defined(ENOTTY) || (ENOTTY != ENOSTR))
        CASE(ENOSTR);
#endif
#ifdef ENOSYM
        CASE(ENOSYM);
#endif
#ifdef ENOSYS
        CASE(ENOSYS);
#endif
#ifdef ENOTBLK
        CASE(ENOTBLK);
#endif
#ifdef ENOTCONN
        CASE(ENOTCONN);
#endif
#ifdef ENOTRECOVERABLE
        CASE(ENOTRECOVERABLE);
#endif
#ifdef ENOTDIR
        CASE(ENOTDIR);
#endif
#if defined(ENOTEMPTY) && (!defined(EEXIST) || (ENOTEMPTY != EEXIST))
        CASE(ENOTEMPTY);
#endif
#ifdef ENOTNAM
        CASE(ENOTNAM);
#endif
#ifdef ENOTSOCK
        CASE(ENOTSOCK);
#endif
#ifdef ENOTSUP
        CASE(ENOTSUP);
#endif
#ifdef ENOTTY
        CASE(ENOTTY);
#endif
#ifdef ENOTUNIQ
        CASE(ENOTUNIQ);
#endif
#ifdef ENXIO
        CASE(ENXIO);
#endif
#if defined(EOPNOTSUPP) &&  (!defined(ENOTSUP) || (ENOTSUP != EOPNOTSUPP))
        CASE(EOPNOTSUPP);
#endif
#ifdef EOTHER
        CASE(EOTHER);
#endif
#if defined(EOVERFLOW) && (!defined(EFBIG) || (EOVERFLOW != EFBIG)) && (!defined(EINVAL) || (EOVERFLOW != EINVAL))
        CASE(EOVERFLOW);
#endif
#ifdef EOWNERDEAD
        CASE(EOWNERDEAD);
#endif
#ifdef EPERM
        CASE(EPERM);
#endif
#if defined(EPFNOSUPPORT) && (!defined(ENOLCK) || (ENOLCK != EPFNOSUPPORT))
        CASE(EPFNOSUPPORT);
#endif
#ifdef EPIPE
        CASE(EPIPE);
#endif
#ifdef EPROCLIM
        CASE(EPROCLIM);
#endif
#ifdef EPROCUNAVAIL
        CASE(EPROCUNAVAIL);
#endif
#ifdef EPROGMISMATCH
        CASE(EPROGMISMATCH);
#endif
#ifdef EPROGUNAVAIL
        CASE(EPROGUNAVAIL);
#endif
#ifdef EPROTO
        CASE(EPROTO);
#endif
#ifdef EPROTONOSUPPORT
        CASE(EPROTONOSUPPORT);
#endif
#ifdef EPROTOTYPE
        CASE(EPROTOTYPE);
#endif
#ifdef ERANGE
        CASE(ERANGE);
#endif
#if defined(EREFUSED) && (!defined(ECONNREFUSED) || (EREFUSED != ECONNREFUSED))
        CASE(EREFUSED);
#endif
#ifdef EREMCHG
        CASE(EREMCHG);
#endif
#ifdef EREMDEV
        CASE(EREMDEV);
#endif
#ifdef EREMOTE
        CASE(EREMOTE);
#endif
#ifdef EREMOTEIO
        CASE(EREMOTEIO);
#endif
#ifdef EREMOTERELEASE
        CASE(EREMOTERELEASE);
#endif
#ifdef EROFS
        CASE(EROFS);
#endif
#ifdef ERPCMISMATCH
        CASE(ERPCMISMATCH);
#endif
#ifdef ERREMOTE
        CASE(ERREMOTE);
#endif
#ifdef ESHUTDOWN
        CASE(ESHUTDOWN);
#endif
#ifdef ESOCKTNOSUPPORT
        CASE(ESOCKTNOSUPPORT);
#endif
#ifdef ESPIPE
        CASE(ESPIPE);
#endif
#ifdef ESRCH
        CASE(ESRCH);
#endif
#ifdef ESRMNT
        CASE(ESRMNT);
#endif
#ifdef ESTALE
        CASE(ESTALE);
#endif
#ifdef ESUCCESS
        CASE(ESUCCESS);
#endif
#if defined(ETIME) && (!defined(ELOOP) || (ETIME != ELOOP))
        CASE(ETIME);
#endif
#if defined(ETIMEDOUT) && (!defined(ENOSTR) || (ETIMEDOUT != ENOSTR))
        CASE(ETIMEDOUT);
#endif
#ifdef ETOOMANYREFS
        CASE(ETOOMANYREFS);
#endif
#ifdef ETXTBSY
        CASE(ETXTBSY);
#endif
#ifdef EUCLEAN
        CASE(EUCLEAN);
#endif
#ifdef EUNATCH
        CASE(EUNATCH);
#endif
#ifdef EUSERS
        CASE(EUSERS);
#endif
#ifdef EVERSION
        CASE(EVERSION);
#endif
#if defined(EWOULDBLOCK) && (!defined(EAGAIN) || (EWOULDBLOCK != EAGAIN))
        CASE(EWOULDBLOCK);
#endif
#ifdef EXDEV
        CASE(EXDEV);
#endif
#ifdef EXFULL
        CASE(EXFULL);
#endif
    default:
        return "UNKNOWN_ERROR";
    }
#undef CASE
}
