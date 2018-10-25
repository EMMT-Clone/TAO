## Communication with the frame grabber

In TAO, a frame grabber is meant to be connected to a, possibly *fake*, camera
and to work as an image server for other processes.  There may be several frame
grabbers running at the same time (they can be distinguished by their names).
To communicate with other processes, [XPA](https://github.com/ericmandel/xpa)
protocol is used to exchange commands and messages in textual form while images
and the global settings of the frame grabber are stored in shared memory.


### Configuration of the camera

When configuring a camera for image acquisition, it is important to distinguish
actual camera settings and chosen but not yet applied parameters.  The semantic
of configuration commands (see below) is that *getting* a parameter shall
always yield the actual parameter value while *setting* a parameter is always
deferred until the configuration settings are *applied*.  In general, applying
the parameters of the configuration cannot be done while image acquisition is
running.  Shared camera data contains the actual parameters and must therefore
be considered as read-only by clients.

| Parameter     | Units        | Configurable | Description                                 |
|:------------- |:------------ |:------------ |:------------------------------------------- |
| `temperature` | °C           | no           | Camera temperature                          |
| `bias`        |              | yes          | Detector Bias                               |
| `gain`        |              | yes          | Detector gain                               |
| `exposure`    | s            | yes          | Exposure time                               |
| `rate`        | Hz           | yes          | Frame rate                                  |
| `fullwidth`   | pixels       | yes          | Width of sensor                             |
| `fullheight`  | pixels       | yes          | Height of sensor                            |
| `xoff`        | pixels       | yes          | Horizontal offset of ROI relative to sensor |
| `yoff`        | pixels       | yes          | Vertical offset of ROI relative to sensor   |
| `width`       | macro-pixels | yes          | Width of acquired images                    |
| `height`      | macro-pixels | yes          | Height of acquired images                   |
| `srcdepth`    | bits         | yes          | Bits per pixel in acquired images           |
| `state`       |              | via commands | Current state of the camera                 |
| `last_ident`  |              | no           | Shared object identifier of last image      |
| `frames`      |              | no           | Number of images acquired so far            |


Here **ROI** means *Region Of Interest*, it is the region corresponding to captured images.


### Available commands

In what follows, the implemented XPA commands are described assuming the client
is operating from the command line (with `xpaget` and `xpaset` commands).  The
client may be coded in any language implementing XPA protocol (as far as I
know: [Julia](http://julialang.org/) with
[XPA.jl](https://github.com/emmt/XPA.jl) package, Yorick, Python, Tcl/Tk,
*etc.*).

Assuming `$cam` is the name of the camera server, for instance
`virtualcamera1`, the command `xpaset` can be used as follows:

```sh
cat data | xpaset TAO:$cam args...
```

to send the contents of `data` to server `TAO:$cam` for arguments
`args...`.

Another possibility is:

```sh
xpaset -p TAO:$cam $param args...
```

to set the value of parameter `$param` (some parameters may take multiple
arguments as their value).

The command `xpaget` can be used as follows:

```sh
xpaget TAO:$cam args... >answer
```

to retrieve some information or some data for command `args...` for server
`TAO:$cam`.  Here the result is written in a file named `answer`.

Assuming `$cam` is the name of the camera server, the following commands
are available (square brackets `[...]` denote optional arguments):

* `xpaset -p TAO:$cam stop` to stop the acquisition after current frame.

* `xpaset -p TAO:$cam abort` to abort the acquisition immediately.

* `xpaset -p TAO:$cam start [$nbufs]` to start the acquisition with `$nbufs`
  image buffers; if `nbufs` is unspecified, a suitable default is used.

* `xpaset -p TAO:$cam quit` to make the camera server exit.

* `xpaset -p TAO:$cam ping` to check the connection.  The result is a
  time-stamp in fractional seconds with nanosecond resolution and using the
  *monotonic* clock whose origin is unspecified but which is insensitive to
  time adjustments by the system.

* `xpaget TAO:$cam debug` to retrieve the current debug mode.  Result is `on`
  or `off`.

* `xpaset -p TAO:$cam debug $bool` to set the debug mode.  Argument `$bool` is
  `on` or `off`.  In debug mode, the server prints the received commands and
  how they are split in words.

* `xpaget TAO:$cam state` to retrieve the current state of the camera.  The
  result is `0` when the device is not yet open, `1` when the device is open
  but no acquisition is running and `2` when an acquisition is running.  To
  change the state, commands `start`, `stop` and `abort` have to be used.

* `xpaget TAO:$cam rate` to retrieve the current acquisition rate
  in frames per second (fps).

* `xpaset -p TAO:$cam rate $fps` to select the acquisition rate of be `$fps`.
  Argument `$fps` can be `max` to choose the maximum rate given the current
  exposure time.  Otherwise, `$fps` is the number of frames per second.
  If current exposure time is too high, it is reduced to the maximum possible
  value.

* `xpaget TAO:$cam exposure` to retrieve the current exposure time in seconds.

* `xpaset -p TAO:$cam exposure $sec` to set the exposure time to be `$sec`.
  Argument `$sec` can be `max` to choose the maximum exposure time given the
  current frame rate.  Otherwise argument `$sec` must be a number of seconds,
  if the frame rate is too high, it is reduced to the maximum possible value.

* `xpaget TAO:$cam roi` to retrieve the current region of interest (ROI) as 4
  integers: `xoff`, `yoff`, `width` and `height`.

* `xpaset -p TAO:$cam roi $xoff $yoff $width $height` to select the region of
  interest (ROI).

* `xpaget TAO:$cam shmid` to retrieve the identifier of the shared memory where
  global camera information is stored.
