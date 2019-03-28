/*
 * andor-errors.c --
 *
 * Management of errors for Andor cameras.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#ifndef _ANDOR_ERRORS_C
#define _ANDOR_ERRORS_C 1

#include "andor.h"

static void
get_andor_error_details(int code, const char** reason, const char** info)
{
    if (reason != NULL) {
        *reason = andor_get_error_reason(code);
    }
    if (info != NULL) {
        *info = andor_get_error_name(code);
    }
}

void
_andor_push_error(tao_error_t** errs, const char* func, int code)
{
    tao_push_other_error(errs, func, code, get_andor_error_details);
}

void
andor_push_error(andor_camera_t* cam, const char* func, int code)
{
    tao_push_other_error(&cam->errs, func, code, get_andor_error_details);
}

#define GET_ERR_FUNC 1
#include __FILE__

#define GET_ERR_FUNC 2
#include __FILE__

#else /* _ANDOR_ERRORS_C defined */

#ifdef GET_ERR_FUNC

#   undef FUNC
#   undef CASE
#   if GET_ERR_FUNC == 1
#       define FUNC       andor_get_error_reason
#       define CASE(id, str) case id: return str
#   elif GET_ERR_FUNC == 2
#       define FUNC       andor_get_error_name
#       define CASE(id, str) case id: return #id
#   else
#       error Invalid value for GET_ERR_FUNC
#   endif

const char*
FUNC(int code)
{
    switch (code) {
        CASE(AT_SUCCESS, "Function call has been successful");
        CASE(AT_ERR_NOTINITIALISED, "Uninitialized handle");
        CASE(AT_ERR_NOTIMPLEMENTED, "Feature is not implemented for the chosen camera");
        CASE(AT_ERR_READONLY, "Feature is read only");
        CASE(AT_ERR_NOTREADABLE, "Feature is currently not readable");
        CASE(AT_ERR_NOTWRITABLE, "Feature is currently not writable/excutable");
        CASE(AT_ERR_OUTOFRANGE, "Feature value is outside valid range");
        CASE(AT_ERR_INDEXNOTAVAILABLE, "Index is currently not available");
        CASE(AT_ERR_INDEXNOTIMPLEMENTED, "Index is not implemented for the chosen camera");
        CASE(AT_ERR_EXCEEDEDMAXSTRINGLENGTH, "String value exceeds maximum allowed length");
        CASE(AT_ERR_CONNECTION, "Error connecting to or disconnecting from hardware");
        CASE(AT_ERR_NODATA, "No data");
        CASE(AT_ERR_INVALIDHANDLE, "Invalid device handle passed to function");
        CASE(AT_ERR_TIMEDOUT, "Time out occured while waiting for data form output queue");
        CASE(AT_ERR_BUFFERFULL, "Input queue has reached its capacity");
        CASE(AT_ERR_INVALIDSIZE, "Queued buffer size does not match frame size");
        CASE(AT_ERR_INVALIDALIGNMENT, "Queued buffer is not aligned on an 8-byte boundary");
        CASE(AT_ERR_COMM, "Error occurred while communicating with hardware");
        CASE(AT_ERR_STRINGNOTAVAILABLE, "Index/string is not available");
        CASE(AT_ERR_STRINGNOTIMPLEMENTED, "Index/string is not implemented for the chosen camera");
        CASE(AT_ERR_NULL_FEATURE, "Null feature name");
        CASE(AT_ERR_NULL_HANDLE, "Null device handle");
        CASE(AT_ERR_NULL_IMPLEMENTED_VAR, "Feature not implemented");
        CASE(AT_ERR_NULL_READABLE_VAR, "Readable not set");
        CASE(AT_ERR_NULL_WRITABLE_VAR, "Writable not set");
        CASE(AT_ERR_NULL_MINVALUE, "Null minimum value");
        CASE(AT_ERR_NULL_MAXVALUE, "Null maximum value");
        CASE(AT_ERR_NULL_VALUE, "Null value returned from function");
        CASE(AT_ERR_NULL_STRING, "Null string returned from function");
        CASE(AT_ERR_NULL_COUNT_VAR, "Null feature count");
        CASE(AT_ERR_NULL_ISAVAILABLE_VAR, "Available not set");
        CASE(AT_ERR_NULL_MAXSTRINGLENGTH, "Null maximum string length");
        CASE(AT_ERR_NULL_EVCALLBACK, "Null EvCallBack parameter");
        CASE(AT_ERR_NULL_QUEUE_PTR, "Null pointer to queue");
        CASE(AT_ERR_NULL_WAIT_PTR, "Null wait pointer");
        CASE(AT_ERR_NULL_PTRSIZE, "Null pointer size");
        CASE(AT_ERR_NOMEMORY, "No memory allocated for current action");
        CASE(AT_ERR_DEVICEINUSE, "Device already being used");
        CASE(AT_ERR_HARDWARE_OVERFLOW, "Software not fast enough to retrieve data from hardware");
    }
#if GET_ERR_FUNC == 1
    return "Unknown error from Andor SDK";
#else
    return "UNKNOWN_ANDOR_SDK_ERROR";
#endif
}

#undef CASE
#undef FUNC
#undef GET_ERR_FUNC

#endif /* GET_ERR_FUNC defined */

#endif /* _ANDOR_ERRORS_C not defined */
