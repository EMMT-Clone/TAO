# Commands

TAO software provides a number of command line tools which are described below.


## Camera configuration

Command `phx_config` can be used to configure a camera.
Its syntax is:

```.sh
phx_config [OPTIONS...] [--] [CAMERA]
```

where `CAMERA` is the identifier of the camera to configure (currently
there is only one camera and this argument should not be specified) and the
following options are available:

* `--roi XOFF,YOFF,WIDTH,HEIGHT` to specify the region of interest.
* `--load ID` to load a given preset configuration.  Argument `ID` is a
  nonnegative number.  `ID=0` correspond to factory settings.
* `--save ID` to save the configuration.  Argument `ID` is a strictly positive
  number (`ID=0` corresponding to factory settings cannnot be overwritten).
* `--depth BITS` to set the number of bits per pixel.
* `--rate FPS` to set the number of frames per second.
* `--exposure TIME` to set the exposure duration in seconds.
* `--bias LEVEL` to set the detector bias (a.k.a. *black level*).
* `--gain VALUE` to set the detector gain.
* `--bitrate VALUE|auto` to set the bitrate of the CoaXPress connection in
  mega-bauds per second (Mbps).  Argument can be `1250`, `2500`, `3125`,
  `5000`, `6250` or `auto`.
* `--channels NUMBER|auto` to set the number of active CoaXPress channels.
  Argument can be `1`, `2`, `4`, or `auto`.
* `--quiet` to use quiet (non-verbose) mode.  If not set, a summary of the
  camera configuration is printed after applying all changes, if any.
* `-h` or `--help` to print a short help and exit.
* The double dash `--` can be used to explicitly mark the end of the options.

If option `--load` is set, then the specified configuration is loaded
**before** applying the other options.  Thus the loaded configuration, if any,
is the camera configuration if no changes are applied.  If option `--load` is
not specified, then the other options will change the camera current
configuration.

If option `--save` is set, then the specified configuration is saved **after**
applying all the other options.

## Examples

To reset the camera to factory settings:
```.sh
phx_config --load 0 [CAMERA]
```

To copy preset configuration 1 as preset configuration 2:
```.sh
phx_config --load 1 --save 2 [CAMERA]
```
