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
static const wchar_t* feature_names[] = {_ANDOR_FEATURES NULL};

const wchar_t**
andor_get_feature_names()
{
    return feature_names;
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
static const andor_feature_type_t simcam_feature_types[] = {
    _ANDOR_FEATURES -1
};

#undef _ANDOR_FEATURE
#define _ANDOR_FEATURE(f,s,z) FEATURE_TYPE(z),
static const andor_feature_type_t zyla_feature_types[] = {
    _ANDOR_FEATURES -1
};

const andor_feature_type_t*
andor_get_simcam_feature_types()
{
    return simcam_feature_types;
}

const andor_feature_type_t*
andor_get_zyla_feature_types()
{
    return zyla_feature_types;
}
