/*
 * andor-features.c --
 *
 * Management of features for Andor cameras.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#include "andor-features.h"

#define STRINGIFY0(x) #x
#define JOIN0(a,b) a##b
#define JOIN(a,b) JOIN(a,b)

#define L(str) L##str

#undef _ANDOR_FEATURE
#define _ANDOR_FEATURE(key, sys, sim, zyl) L(#key),
const wchar_t* andor_feature_names[] = {_ANDOR_FEATURES NULL};

const wchar_t**
andor_get_feature_names()
{
    return andor_feature_names;
}

#define FEATURE_TYPE(x) FEATURE_TYPE_##x

#define FEATURE_TYPE_X ANDOR_NOT_IMPLEMENTED
#define FEATURE_TYPE_B ANDOR_BOOLEAN
#define FEATURE_TYPE_I ANDOR_INTEGER
#define FEATURE_TYPE_F ANDOR_FLOAT
#define FEATURE_TYPE_E ANDOR_ENUMERATED
#define FEATURE_TYPE_S ANDOR_STRING
#define FEATURE_TYPE_C ANDOR_COMMAND

#undef _ANDOR_FEATURE
#define _ANDOR_FEATURE(key, sys, sim, zyl) FEATURE_TYPE(sim),
const andor_feature_type_t andor_simcam_feature_types[] = {
    _ANDOR_FEATURES -1
};

#undef _ANDOR_FEATURE
#define _ANDOR_FEATURE(key, sys, sim, zyl) FEATURE_TYPE(zyl),
const andor_feature_type_t andor_zyla_feature_types[] = {
    _ANDOR_FEATURES -1
};

const andor_feature_type_t*
andor_get_simcam_feature_types()
{
    return andor_simcam_feature_types;
}

const andor_feature_type_t*
andor_get_zyla_feature_types()
{
    return andor_zyla_feature_types;
}

/* Fancy helpers for getters and setters.  A first level of macros (prefixed by
   an underscore) is used to check the key index and to convert it into a
   (wide) string using the global table `andor_feature_names`.  A second level
   of macros is used to report errors.  The number of arguments to the Andor
   Core Library function gives the suffix of the macros (the 2 first arguments
   being the camera handle and the feature key).  */

#define _Call3(func, handle, key, arg3)                         \
    (((key) >= 0 && (key) < ANDOR_NFEATURES) ?                  \
     AT_##func(handle, andor_feature_names[key], arg3) :        \
     AT_ERR_NOTIMPLEMENTED)

#define Call3(func, errs, handle, key, arg3, info)      \
    do {                                                \
        int status = _Call3(func, handle, key, arg3);   \
        if (status != AT_SUCCESS) {                     \
            _andor_push_error(errs, info, status);      \
            return -1;                                  \
        }                                               \
    } while (false)

#define _Call4(func, handle, key, arg3, arg4)                   \
    (((key) >= 0 && (key) < ANDOR_NFEATURES) ?                  \
     AT_##func(handle, andor_feature_names[key], arg3, arg4) :  \
     AT_ERR_NOTIMPLEMENTED)

#define Call4(func, errs, handle, key, arg3, arg4, info)        \
    do {                                                        \
        int status = _Call4(func, handle, key, arg3, arg4);     \
        if (status != AT_SUCCESS) {                             \
            _andor_push_error(errs, info, status);              \
            return -1;                                          \
        }                                                       \
    } while (false)

#define _Call5(func, handle, key, arg3, arg4, arg5)                     \
    (((key) >= 0 && (key) < ANDOR_NFEATURES) ?                          \
     AT_##func(handle, andor_feature_names[key], arg3, arg4, arg5) :    \
     AT_ERR_NOTIMPLEMENTED)

#define Call5(func, errs, handle, key, arg3, arg4, arg5, info)          \
    do {                                                                \
        int status = _Call5(func, handle, key, arg3, arg4, arg5);       \
        if (status != AT_SUCCESS) {                                     \
            _andor_push_error(errs, info, status);                      \
            return -1;                                                  \
        }                                                               \
    } while (false)


int
_andor_is_implemented(tao_error_t** errs, AT_H handle, andor_feature_t key,
                      bool* ptr, const char* info)
{
    AT_BOOL val;
    Call3(IsImplemented, errs, handle, key, &val, info);
    *ptr = (val != AT_FALSE ? true : false);
    return 0;
}

int
_andor_is_readable(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   bool* ptr, const char* info)
{
    AT_BOOL val;
    Call3(IsReadable, errs, handle, key, &val, info);
    *ptr = (val != AT_FALSE ? true : false);
    return 0;
}

int
_andor_is_writable(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   bool* ptr, const char* info)
{
    AT_BOOL val;
    Call3(IsWritable, errs, handle, key, &val, info);
    *ptr = (val != AT_FALSE ? true : false);
    return 0;
}

int
_andor_is_readonly(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   bool* ptr, const char* info)
{
    AT_BOOL val;
    Call3(IsReadOnly, errs, handle, key, &val, info);
    *ptr = (val != AT_FALSE ? true : false);
    return 0;
}

int
_andor_set_boolean(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   bool val, const char* info)
{
    Call3(SetBool, errs, handle, key, (val ? AT_TRUE : AT_FALSE), info);
    return 0;
}

int
_andor_get_boolean(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   bool* ptr, const char* info)
{
    AT_BOOL val;
    Call3(GetBool, errs, handle, key, &val, info);
    *ptr = (val != AT_FALSE ? true : false);
    return 0;
}

int
_andor_set_integer(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   long val, const char* info)
{
    Call3(SetInt, errs, handle, key, (AT_64)val, info);
    return 0;
}

int
_andor_get_integer(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   long* ptr, const char* info)
{
    AT_64 val;
    Call3(GetInt, errs, handle, key, &val, info);
    *ptr = val;
    return 0;
}

int
_andor_get_integer_min(tao_error_t** errs, AT_H handle, andor_feature_t key,
                       long* ptr, const char* info)
{
    AT_64 val;
    Call3(GetIntMin, errs, handle, key, &val, info);
    *ptr = val;
    return 0;
}

int
_andor_get_integer_max(tao_error_t** errs, AT_H handle, andor_feature_t key,
                       long* ptr, const char* info)
{
    AT_64 val;
    Call3(GetIntMax, errs, handle, key, &val, info);
    *ptr = val;
    return 0;
}

int
_andor_set_float(tao_error_t** errs, AT_H handle, andor_feature_t key,
                 double val, const char* info)
{
    Call3(SetFloat, errs, handle, key, val, info);
    return 0;
}

int
_andor_get_float(tao_error_t** errs, AT_H handle, andor_feature_t key,
                 double* ptr, const char* info)
{
    Call3(GetFloat, errs, handle, key, ptr, info);
    return 0;
}

int
_andor_get_float_min(tao_error_t** errs, AT_H handle, andor_feature_t key,
                     double* ptr, const char* info)
{
    Call3(GetFloatMin, errs, handle, key, ptr, info);
    return 0;
}

int
_andor_get_float_max(tao_error_t** errs, AT_H handle, andor_feature_t key,
                     double* ptr, const char* info)
{
    Call3(GetFloatMax, errs, handle, key, ptr, info);
    return 0;
}

int
_andor_set_enum_index(tao_error_t** errs, AT_H handle, andor_feature_t key,
                      int val, const char* info)
{
    Call3(SetEnumIndex, errs, handle, key, val, info);
    return 0;
}

int
_andor_set_enum_string(tao_error_t** errs, AT_H handle, andor_feature_t key,
                       const AT_WC* val, const char* info)
{
    Call3(SetEnumString, errs, handle, key, val, info);
    return 0;
}

int
_andor_get_enum_index(tao_error_t** errs, AT_H handle, andor_feature_t key,
                      int* ptr, const char* info)
{
    Call3(GetEnumIndex, errs, handle, key, ptr, info);
    return 0;
}

int _andor_get_enum_string(tao_error_t** errs, AT_H handle,
                           andor_feature_t key, int idx,
                           AT_WC* val, long len, const char* info)
{
    Call5(GetEnumStringByIndex, errs, handle, key, idx, val, len, info);
    return 0;
}

int
_andor_get_enum_count(tao_error_t** errs, AT_H handle, andor_feature_t key,
                      int* ptr, const char* info)
{
    Call3(GetEnumCount, errs, handle, key, ptr, info);
    return 0;
}

int
_andor_is_enum_index_available(tao_error_t** errs, AT_H handle,
                               andor_feature_t key, int idx,
                               bool* ptr, const char* info)
{
    AT_BOOL val;
    Call4(IsEnumIndexAvailable, errs, handle, key, idx, &val, info);
    *ptr = (val != AT_FALSE ? true : false);
    return 0;
}

int
_andor_is_enum_index_implemented(tao_error_t** errs, AT_H handle,
                                 andor_feature_t key, int idx,
                                 bool* ptr, const char* info)
{
    AT_BOOL val;
    Call4(IsEnumIndexImplemented, errs, handle, key, idx, &val, info);
    *ptr = (val != AT_FALSE ? true : false);
    return 0;
}

int _andor_set_string(tao_error_t** errs, AT_H handle, andor_feature_t key,
                      const AT_WC* val, const char* info)
{
    Call3(SetString, errs, handle, key, val, info);
    return 0;
}

int _andor_get_string(tao_error_t** errs, AT_H handle, andor_feature_t key,
                      AT_WC* str, long len, const char* info)
{
    Call4(GetString, errs, handle, key, str, len, info);
    return 0;
}

int
_andor_get_string_max_length(tao_error_t** errs, AT_H handle,
                             andor_feature_t key, long* ptr,
                             const char* info)
{
    int val;
    Call3(GetStringMaxLength, errs, handle, key, &val, info);
    *ptr = val;
    return 0;
}
