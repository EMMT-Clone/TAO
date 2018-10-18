srcdir=.

CC = gcc
CFLAGS = -O2
CPPFLAGS = $(LIBTAO_DEFS) $(LIBXPA_DEFS)
LDFLAGS = $(LIBTAO_LIBS) $(LIBXPA_LIBS) -lm

LIBTAO_DEFS = -I${srcdir}/../libtao/src
#LIBTAO_LIBS = -L${srcdir}/../build/src/.libs/ -ltao
LIBTAO_LIBS = ${srcdir}/../build/src/.libs/libtao.a -lpthread

LIBXPA_DEFS =
LIBXPA_LIBS = -lxpa

default: all
all: virtualcamera

virtualcamera: virtualcamera.c
	$(CC) $(CPPFLAGS) $(CFLAGS) "$<" -o "$@" $(LDFLAGS)
