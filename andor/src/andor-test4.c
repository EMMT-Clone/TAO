/*
 * andor-test4.c --
 *
 * Simple tests for Andor cameras library: list all features and values.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <andor.h>
#include <andor-features.h>

static void print_feature(FILE* output, AT_H handle,
                          const AT_WC* key, bool skip)
{
    bool is_implemented;
    AT_64 ival;
    AT_BOOL bval;
    double fval;
    int idx;
    int status;
    const int wstrlen = 1024;
    AT_WC wstr[wstrlen];

    status = AT_IsImplemented(handle, key, &bval);
    is_implemented = (status == AT_SUCCESS && bval == AT_TRUE);
    if (skip && !is_implemented) {
        return;
    }
    fprintf(output, "  %ls:\n", key);
    fprintf(output, "    - is implemented: %s\n",
            (status != AT_SUCCESS) ? andor_get_error_name(status) :
            (bval != AT_FALSE ? "true" : "false"));

    status = AT_IsReadOnly(handle, key, &bval);
    fprintf(output, "    - is readonly: %s\n",
            (status != AT_SUCCESS) ? andor_get_error_name(status) :
            (bval != AT_FALSE ? "true" : "false"));
    status = AT_IsReadable(handle, key, &bval);
    fprintf(output, "    - is readable: %s\n",
            (status != AT_SUCCESS) ? andor_get_error_name(status) :
            (bval != AT_FALSE ? "true" : "false"));
    status = AT_IsWritable(handle, key, &bval);
    fprintf(output, "    - is writable: %s\n",
            (status != AT_SUCCESS) ? andor_get_error_name(status) :
            (bval != AT_FALSE ? "true" : "false"));
    if (AT_GetBool(handle, key, &bval) == AT_SUCCESS) {
        fprintf(output, "    - boolean value: %s\n",
                (bval != AT_FALSE ? "true" : "false"));
    }
    if (AT_GetInt(handle, key, &ival) == AT_SUCCESS) {
        fprintf(output, "    - integer value: %ld\n", (long)ival);
    }
    if (AT_GetFloat(handle, key, &fval) == AT_SUCCESS) {
        fprintf(output, "    - floating-point value: %g\n", fval);
    }
    if (AT_GetString(handle, key, wstr, wstrlen) == AT_SUCCESS) {
        fprintf(output, "    - string value: \"%ls\"\n", wstr);
    }
    if (AT_GetEnumIndex(handle, key, &idx) == AT_SUCCESS) {
        fprintf(output, "    - enum. index: %d\n", idx);
        if (AT_GetEnumStringByIndex(handle, key, idx, wstr,
                                    wstrlen) == AT_SUCCESS) {
            fprintf(output, "    - enum. string: \"%ls\"\n", wstr);
        }
    }
}

int main(int argc, char* argv[])
{
    const wchar_t** feature_names = NULL;
    andor_camera_t* cam = NULL;
    tao_error_t* errs = TAO_NO_ERRORS;
    AT_H handle = AT_HANDLE_SYSTEM;
    bool skip = true, optional = true;
    long ndevices;

    ndevices = andor_get_ndevices(&errs);
    if (ndevices < 0) {
        tao_report_errors(&errs);
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; ++i) {
        if (optional && strcmp(argv[i], "-debug") == 0) {
            skip = false;
        } else if (optional && strcmp(argv[i], "--") == 0) {
            optional = false;
        } else if (cam == NULL) {
            char* end;
            long dev = strtol(argv[i], &end, 0);
            if (end == argv[i] || *end != '\0') {
                goto usage;
            }
            if (dev < 0 || dev >= ndevices) {
                fprintf(stderr, "Invalid device number %ld\n", dev);
                return EXIT_FAILURE;
            }
            cam = andor_open_camera(&errs, dev);
            if (cam == NULL) {
                tao_report_errors(&errs);
                return EXIT_FAILURE;
            }
            handle = cam->handle;
            optional = false;
        } else {
        usage:
            fprintf(stderr, "Usage: %s [-debug] [--] [dev]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    feature_names = andor_get_feature_names();
    for (long k = 0; feature_names[k] != NULL; ++k) {
        print_feature(stdout, handle, feature_names[k], skip);
    }

    if (cam != NULL) {
        andor_close_camera(cam);
    }
    tao_discard_errors(&errs);
    return EXIT_SUCCESS;
}
