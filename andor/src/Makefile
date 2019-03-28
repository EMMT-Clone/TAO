srcdir=.

CC = gcc
CPPFLAGS = -I${srcdir} ${TAO_DEFS} ${AT_DEFS}
CFLAGS = -Wall -Werror -O2 -fPIC
LDFLAGS = ${LIBRARY} ${AT_LIBS} ${TAO_LIBS}

#SIMDFLAGS = `${srcdir}/../../libtao/tools/getsimdflags`
SIMDFLAGS = ${shell ${srcdir}/../../libtao/tools/getsimdflags}

AT_DIR = /apps/libexec/andor
AT_INCDIR = ${AT_DIR}/include
AT_LIBDIR = ${AT_DIR}/lib
AT_DEFS = -I${AT_INCDIR}
AT_LIBS = -L${AT_LIBDIR} -latcore

TAO_SRCDIR = ${srcdir}/../../libtao/src
TAO_DEFS = -I${TAO_SRCDIR}
TAO_LIBS = ${srcdir}/../../build/src/.libs/libtao.a -lpthread -lm

LIBRARY = libandor.a
OBJECTS = andor-errors.o andor-core.o andor-features.o andor-convert.o
DEPS = ${srcdir}/andor.h ${srcdir}/andor-features.h ${TAO_SRCDIR}/tao.h

all: ${LIBRARY}

clean:
	rm -f *~ core

distclean: clean
	rm -f ${LIBRARY} ${OBJECTS}

tests: test1 test2 test3 test4

test1: andor-test1
	LD_LIBRARY_PATH=${AT_LIBDIR} ./andor-test1

test2: andor-test2
	LD_LIBRARY_PATH=${AT_LIBDIR} ./andor-test2
	LD_LIBRARY_PATH=${AT_LIBDIR} ./andor-test2 zyla
	LD_LIBRARY_PATH=${AT_LIBDIR} ./andor-test2 simcam

test3: andor-test3
	LD_LIBRARY_PATH=${AT_LIBDIR} ./andor-test3

test4: andor-test4
	LD_LIBRARY_PATH=${AT_LIBDIR} ./andor-test4

andor-test1: ${srcdir}/andor-test1.c ${DEPS} ${LIBRARY}
	${CC} ${CPPFLAGS} ${CFLAGS} $< -o $@ ${LDFLAGS}

andor-test2: ${srcdir}/andor-test2.c ${DEPS} ${LIBRARY}
	${CC} ${CPPFLAGS} ${CFLAGS} $< -o $@ ${LDFLAGS}

andor-test3: ${srcdir}/andor-test3.c ${DEPS} ${LIBRARY}
	${CC} ${CPPFLAGS} ${CFLAGS} $< -o $@ ${LDFLAGS}

andor-test4: ${srcdir}/andor-test4.c ${DEPS} ${LIBRARY}
	${CC} ${CPPFLAGS} ${CFLAGS} $< -o $@ ${LDFLAGS}

${LIBRARY}: ${OBJECTS}
	ar -rf $@ $^

%.o: ${srcdir}/%.c ${DEPS}
	${CC} -c ${CPPFLAGS} ${CFLAGS} $< -o $@

andor-convert.o: ${srcdir}/andor-convert.c ${DEPS}
	${CC} -c ${CPPFLAGS} ${CFLAGS} ${SIMDFLAGS} $< -o $@

.PHOONY: all default clean distclean tests test1 test2 test3
