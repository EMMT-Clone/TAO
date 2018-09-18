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
#include <locale.h>

#include "tao-private.h"

#define USE_STRERROR 0 /* FIXME: should be in config.h */

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

int
tao_pop_error(tao_error_t** errs, const char** funcptr, int* codeptr)
{
    if (errs == NULL || *errs == TAO_NO_ERRORS) {
        return 0;
    } else {
        tao_error_t* err = *errs;
        *errs = err->prev;
        if (funcptr != NULL) {
            *funcptr = err->func;
        }
        if (codeptr != NULL) {
            *codeptr = err->code;
        }
        free((void*)err);
        return 1;
    }
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
#define CASE(c, s) case c: return s
    switch (code) {
        CASE(TAO_CANT_TRACK_ERROR, "Insufficient memory for tracking error");
        CASE(TAO_SYSTEM_ERROR,     "Unknown system error");
        CASE(TAO_DESTROYED,        "Ressource has been destroyed");
        CASE(TAO_BAD_RANK,         "Invalid number of dimensions");
        CASE(TAO_BAD_TYPE,         "Invalid type");
        CASE(TAO_BAD_SIZE,         "Invalid size");
        CASE(TAO_BAD_SERIAL,       "Invalid serial number");
        CASE(TAO_BAD_MAGIC,        "Invalid magic number");
        CASE(TAO_SUCCESS,          "Operation was successful");
#ifdef EPERM
        CASE(EPERM, "Operation not permitted");
#endif
#ifdef ENOENT
        CASE(ENOENT, "No such file or directory");
#endif
#ifdef ESRCH
        CASE(ESRCH, "No such process");
#endif
#ifdef EINTR
        CASE(EINTR, "Interrupted system call");
#endif
#ifdef EIO
        CASE(EIO, "I/O error");
#endif
#ifdef ENXIO
        CASE(ENXIO, "No such device or address");
#endif
#if defined(E2BIG) && (!defined(EOVERFLOW) || (E2BIG != EOVERFLOW))
        CASE(E2BIG, "Argument list too long");
#endif
#ifdef ENOEXEC
        CASE(ENOEXEC, "Exec format error");
#endif
#ifdef EBADF
        CASE(EBADF, "Bad file number");
#endif
#ifdef ECHILD
        CASE(ECHILD, "No child processes");
#endif
#ifdef EAGAIN
        CASE(EAGAIN, "Resource temporarily unavailable");
#endif
#ifdef ENOMEM
        CASE(ENOMEM, "Not enough memory");
#endif
#ifdef EACCES
        CASE(EACCES, "Permission denied");
#endif
#ifdef EFAULT
        CASE(EFAULT, "Bad address");
#endif
#ifdef ENOTBLK
        CASE(ENOTBLK, "Block device required");
#endif
#ifdef EBUSY
        CASE(EBUSY, "Device or resource busy");
#endif
#ifdef EEXIST
        CASE(EEXIST, "File already exists");
#endif
#ifdef EXDEV
        CASE(EXDEV, "Cross-device link");
#endif
#ifdef ENODEV
        CASE(ENODEV, "No such device");
#endif
#ifdef ENOTDIR
        CASE(ENOTDIR, "Not a directory");
#endif
#ifdef EISDIR
        CASE(EISDIR, "Illegal operation on a directory");
#endif
#ifdef EINVAL
        CASE(EINVAL, "Invalid argument");
#endif
#ifdef ENFILE
        CASE(ENFILE, "File table overflow");
#endif
#ifdef EMFILE
        CASE(EMFILE, "Too many open files");
#endif
#ifdef ENOTTY
        CASE(ENOTTY, "Not a typewriter");
#endif
#ifdef ETXTBSY
        CASE(ETXTBSY, "Text file or pseudo-device busy");
#endif
#ifdef EFBIG
        CASE(EFBIG, "File too large");
#endif
#ifdef ENOSPC
        CASE(ENOSPC, "No space left on device");
#endif
#ifdef ESPIPE
        CASE(ESPIPE, "Invalid seek");
#endif
#ifdef EROFS
        CASE(EROFS, "Read-only file system");
#endif
#ifdef EMLINK
        CASE(EMLINK, "Too many links");
#endif
#ifdef EPIPE
        CASE(EPIPE, "Broken pipe");
#endif
#ifdef EDOM
        CASE(EDOM, "Math argument out of function domain");
#endif
#ifdef ERANGE
        CASE(ERANGE, "Math result not representable");
#endif
#ifdef EADDRINUSE
        CASE(EADDRINUSE, "Address already in use");
#endif
#ifdef EADDRNOTAVAIL
        CASE(EADDRNOTAVAIL, "Cannot assign requested address");
#endif
#ifdef EADV
        CASE(EADV, "Advertise error");
#endif
#ifdef EAFNOSUPPORT
        CASE(EAFNOSUPPORT, "Address family not supported by protocol");
#endif
#ifdef EALIGN
        CASE(EALIGN, "EALIGN");
#endif
#if defined(EALREADY) && (!defined(EBUSY) || (EALREADY != EBUSY))
        CASE(EALREADY, "Operation already in progress");
#endif
#ifdef EBADE
        CASE(EBADE, "Bad exchange descriptor");
#endif
#ifdef EBADFD
        CASE(EBADFD, "File descriptor in bad state");
#endif
#ifdef EBADMSG
        CASE(EBADMSG, "Not a data message");
#endif
#ifdef ECANCELED
        CASE(ECANCELED, "Operation canceled");
#endif
#ifdef EBADR
        CASE(EBADR, "Bad request descriptor");
#endif
#ifdef EBADRPC
        CASE(EBADRPC, "RPC structure is bad");
#endif
#ifdef EBADRQC
        CASE(EBADRQC, "Bad request code");
#endif
#ifdef EBADSLT
        CASE(EBADSLT, "Invalid slot");
#endif
#ifdef EBFONT
        CASE(EBFONT, "Bad font file format");
#endif
#ifdef ECHRNG
        CASE(ECHRNG, "Channel number out of range");
#endif
#ifdef ECOMM
        CASE(ECOMM, "Communication error on send");
#endif
#ifdef ECONNABORTED
        CASE(ECONNABORTED, "Software caused connection abort");
#endif
#ifdef ECONNREFUSED
        CASE(ECONNREFUSED, "Connection refused");
#endif
#ifdef ECONNRESET
        CASE(ECONNRESET, "Connection reset by peer");
#endif
#if defined(EDEADLK) && (!defined(EWOULDBLOCK) || (EDEADLK != EWOULDBLOCK))
        CASE(EDEADLK, "Resource deadlock avoided");
#endif
#if defined(EDEADLOCK) && (!defined(EDEADLK) || (EDEADLOCK != EDEADLK))
        CASE(EDEADLOCK, "Resource deadlock avoided");
#endif
#ifdef EDESTADDRREQ
        CASE(EDESTADDRREQ, "Destination address required");
#endif
#ifdef EDIRTY
        CASE(EDIRTY, "Mounting a dirty fs w/o force");
#endif
#ifdef EDOTDOT
        CASE(EDOTDOT, "Cross mount point");
#endif
#ifdef EDQUOT
        CASE(EDQUOT, "Disk quota exceeded");
#endif
#ifdef EDUPPKG
        CASE(EDUPPKG, "Duplicate package name");
#endif
#ifdef EHOSTDOWN
        CASE(EHOSTDOWN, "Host is down");
#endif
#ifdef EHOSTUNREACH
        CASE(EHOSTUNREACH, "Host is unreachable");
#endif
#if defined(EIDRM) && (!defined(EINPROGRESS) || (EIDRM != EINPROGRESS))
        CASE(EIDRM, "Identifier removed");
#endif
#ifdef EINIT
        CASE(EINIT, "Initialization error");
#endif
#ifdef EINPROGRESS
        CASE(EINPROGRESS, "Operation now in progress");
#endif
#ifdef EISCONN
        CASE(EISCONN, "Socket is already connected");
#endif
#ifdef EISNAME
        CASE(EISNAM, "Is a name file");
#endif
#ifdef ELBIN
        CASE(ELBIN, "ELBIN");
#endif
#ifdef EL2HLT
        CASE(EL2HLT, "Level 2 halted");
#endif
#ifdef EL2NSYNC
        CASE(EL2NSYNC, "Level 2 not synchronized");
#endif
#ifdef EL3HLT
        CASE(EL3HLT, "Level 3 halted");
#endif
#ifdef EL3RST
        CASE(EL3RST, "Level 3 reset");
#endif
#ifdef ELIBACC
        CASE(ELIBACC, "Cannot access a needed shared library");
#endif
#ifdef ELIBBAD
        CASE(ELIBBAD, "Accessing a corrupted shared library");
#endif
#ifdef ELIBEXEC
        CASE(ELIBEXEC, "Cannot exec a shared library directly");
#endif
#if defined(ELIBMAX) && (!defined(ECANCELED) || (ELIBMAX != ECANCELED))
        CASE (ELIBMAX,
              "Attempting to link in more shared libraries than system limit");
#endif
#ifdef ELIBSCN
        CASE(ELIBSCN, "Corrupted .lib section in a.out");
#endif
#ifdef ELNRNG
        CASE(ELNRNG, "Link number out of range");
#endif
#if defined(ELOOP) && (!defined(ENOENT) || (ELOOP != ENOENT))
        CASE(ELOOP, "Too many levels of symbolic links");
#endif
#ifdef EMSGSIZE
        CASE(EMSGSIZE, "Message too long");
#endif
#ifdef EMULTIHOP
        CASE(EMULTIHOP, "Multihop attempted");
#endif
#ifdef ENAMETOOLONG
        CASE(ENAMETOOLONG, "File name too long");
#endif
#ifdef ENAVAIL
        CASE(ENAVAIL, "Not available");
#endif
#ifdef ENET
        CASE(ENET, "ENET");
#endif
#ifdef ENETDOWN
        CASE(ENETDOWN, "Network is down");
#endif
#ifdef ENETRESET
        CASE(ENETRESET, "Network dropped connection on reset");
#endif
#ifdef ENETUNREACH
        CASE(ENETUNREACH, "Network is unreachable");
#endif
#ifdef ENOANO
        CASE(ENOANO, "Anode table overflow");
#endif
#if defined(ENOBUFS) && (!defined(ENOSR) || (ENOBUFS != ENOSR))
        CASE(ENOBUFS, "No buffer space available");
#endif
#ifdef ENOCSI
        CASE(ENOCSI, "No CSI structure available");
#endif
#if defined(ENODATA) && (!defined(ECONNREFUSED) || (ENODATA != ECONNREFUSED))
        CASE(ENODATA, "No data available");
#endif
#ifdef ENOLCK
        CASE(ENOLCK, "No locks available");
#endif
#ifdef ENOLINK
        CASE(ENOLINK, "Link has been severed");
#endif
#ifdef ENOMSG
        CASE(ENOMSG, "No message of desired type");
#endif
#ifdef ENONET
        CASE(ENONET, "Machine is not on the network");
#endif
#ifdef ENOPKG
        CASE(ENOPKG, "Package not installed");
#endif
#ifdef ENOPROTOOPT
        CASE(ENOPROTOOPT, "Bad protocol option");
#endif
#if defined(ENOSR) && (!defined(ENAMETOOLONG) || (ENAMETOOLONG != ENOSR))
        CASE(ENOSR, "Out of stream resources");
#endif
#if defined(ENOSTR) && (!defined(ENOTTY) || (ENOTTY != ENOSTR))
        CASE(ENOSTR, "Not a stream device");
#endif
#ifdef ENOSYM
        CASE(ENOSYM, "Unresolved symbol name");
#endif
#ifdef ENOSYS
        CASE(ENOSYS, "Function not implemented");
#endif
#ifdef ENOTCONN
        CASE(ENOTCONN, "Socket is not connected");
#endif
#ifdef ENOTRECOVERABLE
        CASE(ENOTRECOVERABLE, "State not recoverable");
#endif
#if defined(ENOTEMPTY) && (!defined(EEXIST) || (ENOTEMPTY != EEXIST))
        CASE(ENOTEMPTY, "Directory not empty");
#endif
#ifdef ENOTNAM
        CASE(ENOTNAM, "Not a name file");
#endif
#ifdef ENOTSOCK
        CASE(ENOTSOCK, "Socket operation on non-socket");
#endif
#ifdef ENOTSUP
        CASE(ENOTSUP, "Operation not supported");
#endif
#ifdef ENOTUNIQ
        CASE(ENOTUNIQ, "Name not unique on network");
#endif
#if defined(EOPNOTSUPP) &&  (!defined(ENOTSUP) || (ENOTSUP != EOPNOTSUPP))
        CASE(EOPNOTSUPP, "Operation not supported on socket");
#endif
#ifdef EOTHER
        CASE(EOTHER, "Other error");
#endif
#if defined(EOVERFLOW) && (!defined(EFBIG) || (EOVERFLOW != EFBIG)) && (!defined(EINVAL) || (EOVERFLOW != EINVAL))
        CASE(EOVERFLOW, "File too big");
#endif
#ifdef EOWNERDEAD
        CASE(EOWNERDEAD, "Owner died");
#endif
#if defined(EPFNOSUPPORT) && (!defined(ENOLCK) || (ENOLCK != EPFNOSUPPORT))
        CASE(EPFNOSUPPORT, "Protocol family not supported");
#endif
#ifdef EPROCLIM
        CASE(EPROCLIM, "Too many processes");
#endif
#ifdef EPROCUNAVAIL
        CASE(EPROCUNAVAIL, "Bad procedure for program");
#endif
#ifdef EPROGMISMATCH
        CASE(EPROGMISMATCH, "Program version wrong");
#endif
#ifdef EPROGUNAVAIL
        CASE(EPROGUNAVAIL, "RPC program not available");
#endif
#ifdef EPROTO
        CASE(EPROTO, "Protocol error");
#endif
#ifdef EPROTONOSUPPORT
        CASE(EPROTONOSUPPORT, "Protocol not supported");
#endif
#ifdef EPROTOTYPE
        CASE(EPROTOTYPE, "Protocol wrong type for socket");
#endif
#if defined(EREFUSED) && (!defined(ECONNREFUSED) || (EREFUSED != ECONNREFUSED))
        CASE(EREFUSED, "EREFUSED");
#endif
#ifdef EREMCHG
        CASE(EREMCHG, "Remote address changed");
#endif
#ifdef EREMDEV
        CASE(EREMDEV, "Remote device");
#endif
#ifdef EREMOTE
        CASE(EREMOTE, "Pathname hit remote file system");
#endif
#ifdef EREMOTEIO
        CASE(EREMOTEIO, "Remote i/o error");
#endif
#ifdef EREMOTERELEASE
        CASE(EREMOTERELEASE, "EREMOTERELEASE");
#endif
#ifdef ERPCMISMATCH
        CASE(ERPCMISMATCH, "RPC version is wrong");
#endif
#ifdef ERREMOTE
        CASE(ERREMOTE, "Object is remote");
#endif
#ifdef ESHUTDOWN
        CASE(ESHUTDOWN, "Cannot send after socket shutdown");
#endif
#ifdef ESOCKTNOSUPPORT
        CASE(ESOCKTNOSUPPORT, "Socket type not supported");
#endif
#ifdef ESRMNT
        CASE(ESRMNT, "srmount error");
#endif
#ifdef ESTALE
        CASE(ESTALE, "Stale remote file handle");
#endif
#ifdef ESUCCESS
        CASE(ESUCCESS, "Error 0");
#endif
#if defined(ETIME) && (!defined(ELOOP) || (ETIME != ELOOP))
        CASE(ETIME, "Timer expired");
#endif
#if defined(ETIMEDOUT) && (!defined(ENOSTR) || (ETIMEDOUT != ENOSTR))
        CASE(ETIMEDOUT, "Connection timed out");
#endif
#ifdef ETOOMANYREFS
        CASE(ETOOMANYREFS, "Too many references: cannot splice");
#endif
#ifdef EUCLEAN
        CASE(EUCLEAN, "Structure needs cleaning");
#endif
#ifdef EUNATCH
        CASE(EUNATCH, "Protocol driver not attached");
#endif
#ifdef EUSERS
        CASE(EUSERS, "Too many users");
#endif
#ifdef EVERSION
        CASE(EVERSION, "Version mismatch");
#endif
#if defined(EWOULDBLOCK) && (!defined(EAGAIN) || (EWOULDBLOCK != EAGAIN))
        CASE(EWOULDBLOCK, "Operation would block");
#endif
#ifdef EXFULL
        CASE(EXFULL, "Message tables full");
#endif
    default:
#if defined(USE_STRERROR) && (USE_STRERROR != 0)
    {
        static int init = 0;
        if (! init) {
            (void)setlocale(LC_ALL, "C");
            init = 1;
        }
	return strerror(code);
    }
#else
        return "Unknown system error";
#endif
    }
#undef CASE
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
