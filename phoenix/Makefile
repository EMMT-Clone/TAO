#
# Makefile --
#
# Rules to build utilities and wrapper for the Phoenix frame grabber library and
# Mikrotron MC408x camera.
#
#-------------------------------------------------------------------------------
#
# Copyright (C) 2017-2018, Éric Thiébaut.
#

srcdir = .

# Compiler and linker settings.
PIC_FLAGS = -fPIC
CC = gcc
CPPFLAGS = -I.
CFLAGS = -O2 -Wall -Werror
LDFLAGS =

# Settings for ActiveSilicon Phoenix library.
PHX_DIR = /usr/local/activesilicon
PHX_LIBDIR = $(PHX_DIR)/lib64
PHX_INCDIR = $(PHX_DIR)/include
PHX_DEFS = -D_PHX_LINUX -I$(PHX_INCDIR)
PHX_DLL = $(PHX_LIBDIR)/libphxapi-x86_64.so
PHX_LIBS = -L$(PHX_LIBDIR) -lphxfb02-x86_64 -lphxapi-x86_64 -lphxbl-x86_64 -lphxil-x86_64 -lphxdl-x86_64 -lrt -ldl

# Settings for TAO library.
TAO_SRCDIR = ${srcdir}/../libtao/src
TAO_LIBDIR = ${srcdir}/../build/src/.libs
TAO_INCDIR = $(TAO_SRCDIR)
TAO_DEFS = -I$(TAO_INCDIR)
TAO_DLL = $(TAO_LIBDIR)/libtao.so
TAO_LIBS = -L$(TAO_LIBDIR) -ltao -lrt -lpthread
TAO_CFLAGS = $(CPPFLAGS) $(CFLAGS) $(PIC_FLAGS)

# Macros for the Phoenix Frame Grabber Library (FGL):
PHOENIX_LIBRARY = libphoenix.a
PHOENIX_DLL = libphoenix.so
PHOENIX_OBJECTS = phoenix.o coaxpress.o MikrotronMC408x.o tao.o
PHOENIX_HEADERS = phoenix-types.h phoenix.h phoenix-private.h \
              coaxpress.h $(TAO_SRCDIR)/tao.h
#MikrotronMC408x.h tao.h
PHOENIX_DEFS = -DPHX_DEBUG -DPHX_VERBOSE
PHOENIX_CFLAGS = $(CPPFLAGS) $(PHX_DEFS) $(TAO_DEFS) $(PHOENIX_DEFS) $(CFLAGS) $(PIC_FLAGS)
PHOENIX_LIBS = $(PHOENIX_LIBRARY) $(PHX_LIBS)

# Settings for te CFITSIO library.
CFITSIO_LIBS = -lcfitsio
CFITSIO_DEFS =

TOOLS = phx_info phx_live phx_capture phx_rt_last phx_rt_wait

default: all

clean:
	rm -f *~ *.o $(TOOLS)

all: $(TOOLS)

$(PHOENIX_LIBRARY): $(PHOENIX_OBJECTS)
	$(AR) rv $@ $^

$(PHOENIX_DLL): $(PHOENIX_OBJECTS)
	$(CC) -Wl,-rpath,$(PHX_LIBDIR) -fPIC -shared $^ -o $@ $(PHX_LIBS)
#	$(CC) -Wl,-rpath,$(PHX_LIBDIR) -Wl,-export-dynamic -Wl,-soname,$(PHOENIX_DLL).1 -fPIC -shared $^ -o $@ $(PHX_LIBS)
#	$(CC) -Wl,-rpath,$(PHX_LIBDIR) -fPIC -shared -L$(PHX_LIBDIR) $^ -o $@ -lphxapi-x86_64

phx_info: phx_info.c $(PHOENIX_HEADERS) $(PHOENIX_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phx_live: phx_live.c $(PHOENIX_HEADERS) $(PHOENIX_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phx_capture: phx_capture.c $(PHOENIX_HEADERS) $(PHOENIX_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(CFITSIO_DEFS) $(LDFLAGS) $< -o $@ \
	    $(PHOENIX_LIBS) $(CFITSIO_LIBS) -lm

phx_rt_last: phx_rt_last.c $(PHOENIX_HEADERS) $(PHOENIX_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phx_rt_wait: phx_rt_wait.c $(PHOENIX_HEADERS) $(PHOENIX_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phoenix.o: phoenix.c $(PHOENIX_HEADERS)
	$(CC) -c $(PHOENIX_CFLAGS) $< -o $@

coaxpress.o: coaxpress.c $(PHOENIX_HEADERS)
	$(CC) -c $(PHOENIX_CFLAGS) $< -o $@

MikrotronMC408x.o: MikrotronMC408x.c $(PHOENIX_HEADERS)
	$(CC) -c $(PHOENIX_CFLAGS) $< -o $@
