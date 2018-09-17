## Installation

You'll need a C compiler supporting [C99
standard](https://en.wikipedia.org/wiki/C99):

```sh
git clone ... "$TAO_SRC_DIR"
cd "$TAO_SRC_DIR"
./bootstrap
```

where `$TAO_SRC_DIR` is the directory of the TAO source files.


```sh
cd "$TAO_SRC_DIR"
./configure ...
make
make install
```

To build TAO into a specific build directory, say `$BUILD_DIR`, just do:

```sh
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
"$TAO_SRC_DIR"/configure ...
make
make install
```
