srcdir=.

CC = gcc
CPPFLAGS = -I${srcdir} ${TAO_DEFS} ${AT_DEFS}
CFLAGS = -Wall -Werror -O2 -fPIC
LDFLAGS = ${LIBRARY} ${AT_LIBS} ${TAO_LIBS}

AT_DIR = /apps/libexec/andor
AT_INCDIR = ${AT_DIR}/include
AT_LIBDIR = ${AT_DIR}/lib
AT_DEFS = -I${AT_INCDIR}
AT_LIBS = -L${AT_LIBDIR} -latcore

TAO_SRCDIR = ${srcdir}/../../libtao/src
TAO_DEFS = -I${TAO_SRCDIR}
TAO_LIBS = ${srcdir}/../../build/src/.libs/libtao.a -lpthread -lm

LIBRARY = libandorcameras.a
OBJECTS = andor-errors.o andor-core.o andor-features.o
DEPS = ${srcdir}/andorcameras.h ${srcdir}/andor-features.h ${TAO_SRCDIR}/tao.h

all: ${LIBRARY}

clean:
	rm -f *~ core

dist-clean: clean
	rm -f ${LIBRARY} ${OBJECTS}

test: andor-test1 andor-test2
	LD_LIBRARY_PATH=${AT_LIBDIR} ./andor-test1
	LD_LIBRARY_PATH=${AT_LIBDIR} ./andor-test2

andor-test1: ${srcdir}/andor-test1.c ${DEPS} ${LIBRARY}
	${CC} ${CPPFLAGS} ${CFLAGS} $< -o $@ ${LDFLAGS}

andor-test2: ${srcdir}/andor-test2.c ${DEPS} ${LIBRARY}
	${CC} ${CPPFLAGS} ${CFLAGS} $< -o $@ ${LDFLAGS}

${LIBRARY}: ${OBJECTS}
	ar -rf $@ $^

%.o: ${srcdir}/%.c ${DEPS}
	${CC} -c ${CPPFLAGS} ${CFLAGS} $< -o $@

.PHOONY: all default clean dist-clean
