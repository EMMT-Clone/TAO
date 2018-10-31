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
TAO_LIBS = -L$(TAO_LIBDIR) -ltao -lpthread -lm
TAO_CFLAGS = $(CPPFLAGS) $(CFLAGS) $(PIC_FLAGS)

# Macros for the Phoenix Frame Grabber Library (FGL):
PHOENIX_STATIC_LIBRARY = libtaophx.a
PHOENIX_SHARED_LIBRARY = libtaophx.so
PHOENIX_OBJECTS = errors.o phoenix.o mikrotron-mc408x.o
PHOENIX_HEADERS = ${srcdir}/phoenix-types.h \
                  ${srcdir}/phoenix.h \
                  ${srcdir}/phoenix-private.h \
                  ${srcdir}/coaxpress.h \
                  $(TAO_SRCDIR)/tao.h
PHOENIX_DEFS = -DPHX_DEBUG -DPHX_VERBOSE
PHOENIX_CFLAGS = $(CPPFLAGS) $(PHX_DEFS) $(TAO_DEFS) $(PHOENIX_DEFS) $(CFLAGS) $(PIC_FLAGS)
PHOENIX_LIBS = $(PHOENIX_STATIC_LIBRARY) $(PHX_LIBS) $(TAO_LIBS)

# Settings for te CFITSIO library.
CFITSIO_LIBS = -lcfitsio
CFITSIO_DEFS =

TOOLS = phx_info phx_config

default: all

clean:
	rm -f *~ *.o test-macros.e

dist-clean: clean
	rm -f $(PHOENIX_STATIC_LIBRARY) $(PHOENIX_SHARED_LIBRARY) $(TOOLS)

all: $(PHOENIX_STATIC_LIBRARY) $(PHOENIX_SHARED_LIBRARY) $(TOOLS)

$(PHOENIX_STATIC_LIBRARY): $(PHOENIX_OBJECTS)
	$(AR) rv $@ $^

$(PHOENIX_SHARED_LIBRARY): $(PHOENIX_OBJECTS)
	$(CC) -Wl,-rpath,$(PHX_LIBDIR) -fPIC -shared $^ -o $@ $(PHX_LIBS)
#	$(CC) -Wl,-rpath,$(PHX_LIBDIR) -Wl,-export-dynamic -Wl,-soname,$(PHOENIX_SHARED_LIBRARY).1 -fPIC -shared $^ -o $@ $(PHX_LIBS)
#	$(CC) -Wl,-rpath,$(PHX_LIBDIR) -fPIC -shared -L$(PHX_LIBDIR) $^ -o $@ -lphxapi-x86_64

phx_info: ${srcdir}/phx_info.c $(PHOENIX_HEADERS) $(PHOENIX_STATIC_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phx_config: ${srcdir}/phx_config.c $(PHOENIX_HEADERS) $(PHOENIX_STATIC_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phx_live: ${srcdir}/phx_live.c $(PHOENIX_HEADERS) $(PHOENIX_STATIC_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phx_capture: ${srcdir}/phx_capture.c $(PHOENIX_HEADERS) $(PHOENIX_STATIC_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(CFITSIO_DEFS) $(LDFLAGS) $< -o $@ \
	    $(PHOENIX_LIBS) $(CFITSIO_LIBS) -lm

phx_rt_last: ${srcdir}/phx_rt_last.c $(PHOENIX_HEADERS) $(PHOENIX_STATIC_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phx_rt_wait: ${srcdir}/phx_rt_wait.c $(PHOENIX_HEADERS) $(PHOENIX_STATIC_LIBRARY)
	$(CC) $(PHOENIX_CFLAGS) $(LDFLAGS) $< -o $@ $(PHOENIX_LIBS)

phoenix.o: ${srcdir}/phoenix.c $(PHOENIX_HEADERS)
	$(CC) -c $(PHOENIX_CFLAGS) $< -o $@

dump-errors: ${srcdir}/dump-errors.c
	$(CC) $(PHX_DEFS) $(CFLAGS) "$<" -o "$@" $(PHX_LIBS)

errors.c: dump-errors
	rm -f "$@"
	LD_LIBRARY_PATH="$(PHX_LIBDIR)" ./dump-errors > "$@"
	chmod 444 "$@"

errors.o: errors.c $(PHOENIX_HEADERS)
	$(CC) -c $(PHOENIX_CFLAGS) $< -o $@

coaxpress.o: ${srcdir}/coaxpress.c $(PHOENIX_HEADERS)
	$(CC) -c $(PHOENIX_CFLAGS) $< -o $@

mikrotron-mc408x.o: ${srcdir}/mikrotron-mc408x.c $(PHOENIX_HEADERS) ${srcdir}/mikrotron-mc408x.h
	$(CC) -c $(PHOENIX_CFLAGS) $< -o $@

test-macros.e: ${srcdir}/test-macros.c ${srcdir}/coaxpress.h ${srcdir}/mikrotron-mc408x.h
	$(CC) -E "$<" -o "$@"

test-macros: test-macros.e
	tail -30 "$<"

.PHONY: test-macros clean dist-clean
