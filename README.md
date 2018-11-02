# A framework for real-time software
[![License](http://img.shields.io/badge/license-MIT-brightgreen.svg?style=flat)](LICENSE.md)
[![Build Status](https://travis-ci.org/emmt/TAO.svg?branch=master)](https://travis-ci.org/emmt/TAO)

TAO was initially designed as a software framework to carry the real-time
processing needed by adaptive optics (AO) systems.

TAO exploits the [XPA](https://github.com/ericmandel/xpa) messaging system for
the communication between clients and servers of the infrastruture.  For fast
exchanges of data, TAO uses [shared objects](./docs/sharedobjects.md) whose
contents is stored in shared memory.  For fast synchronization, TAO uses
semaphores.

## Documentation

* [Shared objects](./docs/sharedobjects.md).
* [Commands](./docs/commands.md).


## Installation

You'll need a C compiler supporting [C99
standard](https://en.wikipedia.org/wiki/C99):

```sh
git clone ... "$TAO_SRC_DIR"
cd "$TAO_SRC_DIR"
./bootstrap
```

where `$TAO_SRC_DIR` is the directory of the TAO source files.  To build TAO
runtime library into its source directory:


```sh
cd "$TAO_SRC_DIR"/libtao
./configure ...
make
make install
```

To build TAO runtime library into a specific build directory, say `$BUILD_DIR`,
just do:

```sh
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
"$TAO_SRC_DIR"/libtao/configure ...
make
make install
```
