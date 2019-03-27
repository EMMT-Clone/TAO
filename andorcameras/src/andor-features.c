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
#define _ANDOR_FEATURE(f,s,z) L(#f),
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
#define _ANDOR_FEATURE(f,s,z) FEATURE_TYPE(s),
const andor_feature_type_t andor_simcam_feature_types[] = {
    _ANDOR_FEATURES -1
};

#undef _ANDOR_FEATURE
#define _ANDOR_FEATURE(f,s,z) FEATURE_TYPE(z),
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

int
_andor_get_boolean(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   bool* ptr, const char* info)
{
    AT_BOOL val;
    int status = (key >= 0 && key < ANDOR_NFEATURES) ?
        AT_GetBool(handle, andor_feature_names[key], &val) :
        AT_ERR_NOTIMPLEMENTED;
    if (status != AT_SUCCESS) {
        andor_push_error(errs, info, status);
        return -1;
    }
    *ptr = (val != AT_FALSE ? true : false);
    return 0;
}

int
_andor_get_integer(tao_error_t** errs, AT_H handle, andor_feature_t key,
                   long* ptr, const char* info)
{
    AT_64 val;
    int status = (key >= 0 && key < ANDOR_NFEATURES) ?
        AT_GetInt(handle, andor_feature_names[key], &val) :
        AT_ERR_NOTIMPLEMENTED;
    if (status != AT_SUCCESS) {
        andor_push_error(errs, info, status);
        return -1;
    }
    *ptr = val;
    return 0;
}

int
_andor_get_float(tao_error_t** errs, AT_H handle, andor_feature_t key,
                 double* ptr, const char* info)
{
    int status = (key >= 0 && key < ANDOR_NFEATURES) ?
        AT_GetFloat(handle, andor_feature_names[key], ptr) :
        AT_ERR_NOTIMPLEMENTED;
    if (status != AT_SUCCESS) {
        andor_push_error(errs, info, status);
        return -1;
    }
    return 0;
}

int
andor_set_integer(andor_camera_t* cam, andor_feature_type_t key,
                  long val)
{
    int status;

    if (key >= 0 && key < ANDOR_NFEATURES) {
        status = AT_SetInt(cam->handle, andor_feature_names[key], val);
    } else {
        status = AT_ERR_NOTIMPLEMENTED;
    }
    if (status != AT_SUCCESS) {
        andor_push_error(&cam->errs, "AT_SetInt", status);
        return -1;
    }
    return 0;
}

int
andor_get_integer(andor_camera_t* cam, andor_feature_type_t key,
                  long* val)
{
    AT_64 ival;
    int status;

    if (key >= 0 && key < ANDOR_NFEATURES) {
        status = AT_GetInt(cam->handle, andor_feature_names[key], &ival);
    } else {
        status = AT_ERR_NOTIMPLEMENTED;
    }
    if (status != AT_SUCCESS) {
        andor_push_error(&cam->errs, "AT_GetInt", status);
        return -1;
    }
    *val = (long)ival;
    return 0;
}

int
andor_set_float(andor_camera_t* cam, andor_feature_type_t key,
                double val)
{
    int status;

    if (key >= 0 && key < ANDOR_NFEATURES) {
        status = AT_SetFloat(cam->handle, andor_feature_names[key], val);
    } else {
        status = AT_ERR_NOTIMPLEMENTED;
    }
    if (status != AT_SUCCESS) {
        andor_push_error(&cam->errs, "AT_SetFloat", status);
        return -1;
    }
    return 0;
}

int
andor_get_float(andor_camera_t* cam, andor_feature_type_t key,
                double* val)
{
    int status;

    if (key >= 0 && key < ANDOR_NFEATURES) {
        status = AT_GetFloat(cam->handle, andor_feature_names[key], val);
    } else {
        status = AT_ERR_NOTIMPLEMENTED;
    }
    if (status != AT_SUCCESS) {
        andor_push_error(&cam->errs, "AT_GetFloat", status);
        return -1;
    }
    return 0;
}
