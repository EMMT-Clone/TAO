/*
 * andor-test2.c --
 *
 * Simple tests for Andor cameras library: list features.
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
#include <andor.h>
#include <andor-features.h>

int main(int argc, char* argv[])
{
    const wchar_t** feature_names = NULL;
    const andor_feature_type_t* feature_types = NULL;
    const char* model = NULL;

    if (argc == 2) {
        model = argv[1];
        if (strcmp(model, "simcam") == 0) {
            feature_types = andor_get_simcam_feature_types();
        } else if (strcmp(model, "zyla") == 0) {
            feature_types = andor_get_zyla_feature_types();
        } else {
            fprintf(stderr, "Unknown camera model \"%s\"\n", model);
            return EXIT_FAILURE;
        }
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [simcam|zyla]\n", argv[0]);
        return EXIT_FAILURE;
    }
    feature_names = andor_get_feature_names();
    if (feature_types == NULL) {
        long nfeatures = 0;
        for (long k = 0; feature_names[k] != NULL; ++k) {
            fprintf(stdout, "%3ld: %ls\n", k, feature_names[k]);
            ++nfeatures;
        }
        if (nfeatures != ANDOR_NFEATURES) {
            fprintf(stderr, "ANDOR_NFEATURES = %d, while %ld features found\n",
                    ANDOR_NFEATURES, nfeatures);
            return EXIT_FAILURE;
        }
        fprintf(stdout, "ANDOR_NFEATURES = %d (OK)\n", ANDOR_NFEATURES);
    } else {
        for (long k = 0; feature_names[k] != NULL; ++k) {
            const char* typename = NULL;
            switch (feature_types[k]) {
            case ANDOR_FEATURE_BOOLEAN:    typename = "Boolean";    break;
            case ANDOR_FEATURE_INTEGER:    typename = "Integer";    break;
            case ANDOR_FEATURE_FLOAT:      typename = "Float";      break;
            case ANDOR_FEATURE_ENUMERATED: typename = "Enumerated"; break;
            case ANDOR_FEATURE_STRING:     typename = "String";     break;
            case ANDOR_FEATURE_COMMAND:    typename = "Command";    break;
            default: typename = NULL;
            }
            if (typename != NULL) {
                fprintf(stdout, "%-30ls %s\n", feature_names[k], typename);
            }
        }
    }
    return EXIT_SUCCESS;
}
