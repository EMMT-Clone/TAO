srcdir=.

CC = gcc
CFLAGS = -O2 -Wall -Werror
CPPFLAGS = -I.
LDFLAGS =
LIBS = -lm

FITSIO_DEFS =
FITSIO_LIBS = -lcfitsio

TAO_DEFS = -I${srcdir}/../libtao/src
#TAO_LIBS = -L${srcdir}/../build/src/.libs/ -ltao
TAO_LIBS = ${srcdir}/../build/src/.libs/libtao.a -lpthread

XPA_DEFS =
XPA_LIBS = -lxpa

TARGETS = virtualcamera test-fits test-endian

default: all
all: $(TARGETS)

clean:
	rm -f *~ *.o $(TARGETS)

virtualcamera: ${srcdir}/virtualcamera.c
	$(CC) $(CPPFLAGS) $(TAO_DEFS) $(XPA_DEFS) $(CFLAGS) $(LDFLAGS) "$<" -o "$@" $(TAO_LIBS) $(XPA_LIBS) $(LIBS)

test-fits: ${srcdir}/test-fits.c
	$(CC) $(CPPFLAGS) $(TAO_DEFS) $(FITSIO_DEFS) $(CFLAGS) $(LDFLAGS) "$<" -o "$@" $(TAO_LIBS) $(FITSIO_LIBS) $(LIBS)

test-endian: ${srcdir}/test-endian.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) "$<" -o "$@"

.PHONY: all clean default
