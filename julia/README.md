# Tao is a Toolkit for Adaptive Optics

Adaptive Optics (AO) software suite provided by the package **Tao** has two
parts:

* `import Tao` or `using Tao` provides module `Tao` with types and methods for
  processing AO data.

* Once the `Tao` module is loaded, call `Tao.loadclib()` to load the interface
  to the TAO C-library which implements the building blocks of real-time AO
  systems such as shared data, interprocess communication and virtual cameras.

This behavior is intended to allow one to use the data processing methods in
`Tao` without the needs to compile and install the C-library (at least because
this library has not not yet been ported to other systems than Linux).


## Installation

In your `~/.julia/config/startup.jl` file, add a line like:

```julia
push!(LOAD_PATH, "$PATH_TO_TAO_JULIA_SRC")
```

where `"$PATH_TO_TAO_JULIA_SRC"` is the path to the directory `julia/src`.
For instance:

```julia
push!(LOAD_PATH, joinpath(ENV["HOME"], "git/tao/julia/src"))
```


## Documentation

The folowing documentation is available:

* [Detector Calibration](docs/detector.md)
