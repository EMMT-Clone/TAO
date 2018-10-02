# TAO is a Toolkit for Adaptive Optics

Adaptive Optics (AO) software suite provided by the package **TAO** has two parts:

* `import TAO` or `using TAO` provides module `TAO` with types and methods for
  processing AO data.

* Once the `TAO` module is loaded, call `TAO.loadclib()` to load the interface
  to the TAO C-library which implements the building blocks of real-time AO
  systems such as shared data, interprocess communication and virtual cameras.

This behavior is intended to allow one to use the data processing methods in
`TAO` without the needs to compile and install the C-library (at least because
this library has not not yet been ported to other systems than Linux).


## Documentation

The folowing documentation is available:

* [Detector Calibration](docs/detector.md)
