#include <stdio.h>
#include <andorcameras.h>

int main(int argc, char* argv[])
{
    tao_error_t* errs = TAO_NO_ERRORS;
    long ndevices = andor_get_ndevices(&errs);
    if (ndevices < 0) {
        tao_report_errors(&errs);
        return EXIT_FAILURE;
    }
    fprintf(stdout, "%ld device(s) found\n", ndevices);
    return EXIT_SUCCESS;
}
