# Prelude.
AC_INIT([tao], [0.0.1], [eric.thiebaut@univ-lyon1.fr])
AC_CONFIG_SRCDIR([src/tao.h])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror dist-bzip2])

# Checks for programs.
AM_PROG_AR
LT_INIT([shared])
AC_PROG_CC_C99

# Turn on all warnings for gcc.  (Options -Wall -Werror in AM_INIT_AUTOMAKE
# have no effects.)
if [[ "$GCC" = "yes" ]]; then
    #CFLAGS="$CFLAGS -Wall -Werror -Wsign-compare -Wconversion"
    CFLAGS="$CFLAGS -Wall -Werror"
fi

# Checks for header files.
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([sys/ipc.h sys/shm.h])
AC_CHECK_HEADERS([semaphore.h])
AC_CHECK_HEADERS([fitsio2.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([stdbool.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_LIB([pthread], [pthread_mutex_lock],
             [PTHREADLIB=-lpthread], [PTHREADLIB=])
AC_SUBST([PTHREADLIB])

# Output files.
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
