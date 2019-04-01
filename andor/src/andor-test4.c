/*
 * andor-test4.c --
 *
 * Tests for Andor cameras library: list all features and values.  The debug
 * mode can be used to print all features even those which are not implemented.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#include <wchar.h>
#include <string.h>
#include <andor.h>
#include <andor-features.h>

static void print_key(FILE* output, const AT_WC* key, long pad, int c)
{
    long len = (key != NULL ? wcslen(key) : 0);
    fprintf(output, "  %ls", (len > 0 ? key : L""));
    if (c == ':') {
        fputc(c, output);
        while (++len <= pad) {
            fputc(' ', output);
        }
    } else if (c == '>') {
        fputc(' ', output);
        while (++len <= pad) {
            fputc('-', output);
        }
        fputc(c, output);
        fputc(' ', output);
    } else if (c == '.') {
        fputc(' ', output);
        while (++len <= pad) {
            fputc('.', output);
        }
        fputc(' ', output);
    } else {
        while (++len <= pad) {
            fputc(' ', output);
        }
        fputc(' ', output);
    }
}

#define PRINT_KEY(file, key)  print_key(file, key, 30, '.')

#define PRINT_FEATURE0(file, key, format)       \
    do {                                        \
        PRINT_KEY(file, key);                   \
        fprintf(file, format);                  \
    } while (false)

#define PRINT_FEATURE1(file, key, format, arg1) \
    do {                                        \
        PRINT_KEY(file, key);                   \
        fprintf(file, format, arg1);            \
    } while (false)

#define PRINT_FEATURE2(file, key, format, arg1, arg2)   \
    do {                                                \
        PRINT_KEY(file, key);                           \
        fprintf(file, format, arg1, arg2);              \
    } while (false)

#define PRINT_FEATURE3(file, key, format, arg1, arg2, arg3)     \
    do {                                                        \
        PRINT_KEY(file, key);                                   \
        fprintf(file, format, arg1, arg2, arg3);                \
    } while (false)

static const char* rwflags(unsigned int mode)
{
    unsigned int m = (mode & (ANDOR_FEATURE_READABLE|ANDOR_FEATURE_WRITABLE));
    return (m == (ANDOR_FEATURE_READABLE|ANDOR_FEATURE_WRITABLE) ? "rw" :
            (m == ANDOR_FEATURE_READABLE ? "r-" :
             ((m) == ANDOR_FEATURE_WRITABLE ? "-w" : "--")));
}

static void fatal_error(const AT_WC* key, int status,
                        const char* file, int line)
{
    fprintf(stderr, "Unexpected error [%s] for \"%ls\" in %s, line %d\n",
            andor_get_error_name(status), key, file, line);
    exit(EXIT_FAILURE);
}

#define FATAL_ERROR(key, status) fatal_error(key, status, __FILE__, __LINE__)

static void ambiguous_type(const AT_WC* key, andor_feature_type_t type,
                           const char* file, int line)
{
    fprintf(stderr, "ambiguous type [%d] for \"%ls\" in %s, line %d\n",
            type, key, file, line);
    exit(EXIT_FAILURE);
}

#define AMBIGUOUS_TYPE(key, type) ambiguous_type(key, type, __FILE__, __LINE__)

#define ASSERT(key, expr)                                               \
    do {                                                                \
        if (!(expr)) {                                                  \
            fprintf(stderr,                                             \
                    "Assertion failed [%s] for \"%ls\" in %s, line %d\n", \
                    #expr, key, __FILE__, __LINE__);                    \
            /*exit(EXIT_FAILURE);*/                                     \
        }                                                               \
    } while (false)

static void print_feature(FILE* output, AT_H handle,
                          const AT_WC* key, bool debug)
{
    AT_64 ival;
    AT_BOOL bval;
    double fval;
    int idx;
    int status;
    unsigned int mode;
    andor_feature_type_t type;
    const int wstrlen = 1024;
    AT_WC wstr[wstrlen];

    if (debug) {
        bool implemented, readable, writable, readonly;

        /* Check that AT_IsImplemented and AT_IsReadable yield the same
           result. */
        status = AT_IsImplemented(handle, key, &bval);
        if (status != AT_SUCCESS) {
            FATAL_ERROR(key, status);
        }
        implemented = (bval != AT_FALSE ? true : false);

        status = AT_IsReadable(handle, key, &bval);
        if (status != (implemented ? AT_SUCCESS : AT_ERR_NOTIMPLEMENTED)) {
            FATAL_ERROR(key, status);
        }
        readable = (bval != AT_FALSE ? true : false);

        status = AT_IsWritable(handle, key, &bval);
        if (status != (implemented ? AT_SUCCESS : AT_ERR_NOTIMPLEMENTED)) {
            FATAL_ERROR(key, status);
        }
        writable = (bval != AT_FALSE ? true : false);

        status = AT_IsReadOnly(handle, key, &bval);
        if (status != (implemented ? AT_SUCCESS : AT_ERR_NOTIMPLEMENTED)) {
            FATAL_ERROR(key, status);
        }
        readonly = (bval != AT_FALSE ? true : false);

        ASSERT(key, implemented == readable);
        ASSERT(key, readable || ! writable);
        ASSERT(key, readonly == (readable && ! writable));
        mode = ((readable ? ANDOR_FEATURE_READABLE : 0)|
                (writable ? ANDOR_FEATURE_WRITABLE : 0));

        type = ANDOR_FEATURE_NOT_IMPLEMENTED;

        status = AT_GetBool(handle, key, &bval);
        if (status == AT_SUCCESS &&
            implemented && type == ANDOR_FEATURE_NOT_IMPLEMENTED) {
            type = ANDOR_FEATURE_BOOLEAN;
            PRINT_FEATURE2(output, key, "%s boolean = %s\n", rwflags(mode),
                           (bval != AT_FALSE ? "true" : "false"));
        } else if (status == AT_SUCCESS &&
                   type != ANDOR_FEATURE_NOT_IMPLEMENTED) {
            AMBIGUOUS_TYPE(key, type);
        } else if (status != AT_ERR_NOTIMPLEMENTED) {
            FATAL_ERROR(key, status);
        }

        status = AT_GetInt(handle, key, &ival);
        if (status == AT_SUCCESS &&
            implemented && type == ANDOR_FEATURE_NOT_IMPLEMENTED) {
            type = ANDOR_FEATURE_INTEGER;
            PRINT_FEATURE2(output, key, "%s integer = %ld\n", rwflags(mode),
                           (long)ival);
        } else if (status == AT_SUCCESS &&
                   type != ANDOR_FEATURE_NOT_IMPLEMENTED) {
            AMBIGUOUS_TYPE(key, type);
        } else if (status != AT_ERR_NOTIMPLEMENTED) {
            FATAL_ERROR(key, status);
        }

        status = AT_GetFloat(handle, key, &fval);
        if (status == AT_SUCCESS &&
            implemented && type == ANDOR_FEATURE_NOT_IMPLEMENTED) {
            type = ANDOR_FEATURE_FLOAT;
            PRINT_FEATURE2(output, key, "%s float = %g\n", rwflags(mode), fval);
        } else if (status == AT_SUCCESS &&
                   type != ANDOR_FEATURE_NOT_IMPLEMENTED) {
            AMBIGUOUS_TYPE(key, type);
        } else if (status != AT_ERR_NOTIMPLEMENTED) {
            FATAL_ERROR(key, status);
        }

        status = AT_GetString(handle, key, wstr, wstrlen);
        if (status == AT_SUCCESS &&
            implemented && type == ANDOR_FEATURE_NOT_IMPLEMENTED) {
            type = ANDOR_FEATURE_STRING;
            PRINT_FEATURE2(output, key, "%s string = \"%ls\"\n", rwflags(mode),
                           wstr);
        } else if (status == AT_SUCCESS &&
                   type != ANDOR_FEATURE_NOT_IMPLEMENTED) {
            AMBIGUOUS_TYPE(key, type);
        } else if (status != AT_ERR_NOTIMPLEMENTED) {
            FATAL_ERROR(key, status);
        }

        status = AT_GetEnumIndex(handle, key, &idx);
        if (status == AT_SUCCESS &&
            implemented && type == ANDOR_FEATURE_NOT_IMPLEMENTED) {
            status = AT_GetEnumStringByIndex(handle, key, idx, wstr, wstrlen);
            if (status != AT_SUCCESS) {
                FATAL_ERROR(key, status);
            }
            type = ANDOR_FEATURE_ENUMERATED;
            PRINT_FEATURE3(output, key, "%s enumerated = %d / \"%ls\"\n",
                           rwflags(mode), idx, wstr);
        } else if (status == AT_SUCCESS &&
                   type != ANDOR_FEATURE_NOT_IMPLEMENTED) {
            AMBIGUOUS_TYPE(key, type);
        } else if (status != AT_ERR_NOTIMPLEMENTED) {
            FATAL_ERROR(key, status);
        }

        if (implemented && type == ANDOR_FEATURE_NOT_IMPLEMENTED) {
            type = ANDOR_FEATURE_COMMAND;
            PRINT_FEATURE1(output, key, "%s command\n", rwflags(mode));
        }

        if (type == ANDOR_FEATURE_NOT_IMPLEMENTED) {
            PRINT_FEATURE0(output, key, "--\n");
        }

        ASSERT(key, implemented == (type != ANDOR_FEATURE_NOT_IMPLEMENTED));
        ASSERT(key, type == _andor_get_feature_type(handle, key, NULL));

    } else {
        /* The following is not very efficient (because is amounts to query the
           value of the feature at least twice) but it is meant to test the
           library. */
        type = _andor_get_feature_type(handle, key, &mode);
        switch (type) {
        case ANDOR_FEATURE_NOT_IMPLEMENTED:
            break;
        case ANDOR_FEATURE_BOOLEAN:
            status = AT_GetBool(handle, key, &bval);
            if (status != AT_SUCCESS) {
                FATAL_ERROR(key, status);
            }
            PRINT_FEATURE2(output, key, "%s boolean = %s\n", rwflags(mode),
                          (bval != AT_FALSE ? "true" : "false"));
            break;
        case ANDOR_FEATURE_INTEGER:
            status = AT_GetInt(handle, key, &ival);
            if (status != AT_SUCCESS) {
                FATAL_ERROR(key, status);
            }
            PRINT_FEATURE2(output, key, "%s integer = %ld\n", rwflags(mode),
                          (long)ival);
            break;
        case ANDOR_FEATURE_FLOAT:
            status = AT_GetFloat(handle, key, &fval);
            if (status != AT_SUCCESS) {
                FATAL_ERROR(key, status);
            }
            PRINT_FEATURE2(output, key, "%s float = %g\n", rwflags(mode), fval);
            break;
        case ANDOR_FEATURE_ENUMERATED:
            status = AT_GetEnumIndex(handle, key, &idx);
            if (status != AT_SUCCESS) {
                FATAL_ERROR(key, status);
            }
            status = AT_GetEnumStringByIndex(handle, key, idx, wstr, wstrlen);
            if (status != AT_SUCCESS) {
                FATAL_ERROR(key, status);
            }
            PRINT_FEATURE3(output, key, "%s enumerated = %d / \"%ls\"\n",
                           rwflags(mode), idx, wstr);
            break;
        case ANDOR_FEATURE_STRING:
            status = AT_GetString(handle, key, wstr, wstrlen);
            if (status != AT_SUCCESS) {
                FATAL_ERROR(key, status);
            }
            PRINT_FEATURE2(output, key, "%s string = \"%ls\"\n", rwflags(mode),
                           wstr);
            break;
        case ANDOR_FEATURE_COMMAND:
            PRINT_FEATURE1(output, key, "%s command\n", rwflags(mode));
            break;
        default:
            fprintf(stderr, "Unexpected type [%d] in %s, line %d\n",
                    type, __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[])
{
    const wchar_t** feature_names = NULL;
    andor_camera_t* cam = NULL;
    tao_error_t* errs = TAO_NO_ERRORS;
    AT_H handle = AT_HANDLE_SYSTEM;
    bool debug = false, optional = true;
    long ndevices;

    ndevices = andor_get_ndevices(&errs);
    if (ndevices < 0) {
        tao_report_errors(&errs);
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; ++i) {
        if (optional && strcmp(argv[i], "-debug") == 0) {
            debug = true;
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
        print_feature(stdout, handle, feature_names[k], debug);
    }

    if (cam != NULL) {
        andor_close_camera(cam);
    }
    tao_discard_errors(&errs);
    return EXIT_SUCCESS;
}
