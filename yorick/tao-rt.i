/*
 * tao-rt.i --
 *
 * Yorick interface to TAO real-time software.  TAO is a library for Adaptive
 * Optics software
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

if (is_func(plug_in)) plug_in, "tao_rt";

func tao_connect_camera(cam)
/* DOCUMENT tao_connect_camera(cam);

     Connects to a TAO shared camera the result is an attached shared camera
     (see tao_attach_shared_camera).  Argument CAM can be a TAO shared camera,
     the shared memory identifier of a TAO shared camera or the XPA access
     point name of an XPA camera server.

   SEE ALSO: tao_attach_shared_camera, tao_query_shmid.
 */
{
    if (is_string(cam)) cam = tao_query_shmid(cam);
    if (is_integer(cam)) cam = tao_attach_shared_camera(cam);
    if (cam.type != TAO_SHARED_CAMERA) error, "not a shared TAO camera";
    return cam;
}

extern tao_get_current_time;
extern tao_get_monotonic_time;
/* DOCUMENT t = tao_get_current_time();
         or t = tao_get_monotonic_time();

     These function yields the time with nanossecond resolution.  The returned
     value is a pair of integers: `t(1)` is a number of seconds, `t(2)` is a
     number of nanoseconds.  The function `tao_get_current_time` yields the
     time since a specified starting point.  The function
     `tao_get_monotonic_time` yields the time since an unspecified starting
     point but which is not affected by discontinuous jumps in the system time
     (e.g., if the system administrator manually changes the clock), but is
     affected by the incremental adjustments performed by adjtime() and NTP.

   SEE ALSO:
 */

local TAO_SHARED_MAGIC, TAO_SHARED_OBJECT, TAO_SHARED_ARRAY, TAO_SHARED_CAMERA;
extern tao_create_shared_object;
/* DOCUMENT obj = tao_create_shared_object(type, size);
         or obj = tao_create_shared_object(type, size, perms);

     Creates a new shared TAO object of given `type` and `size` (in bytes)
     with access permissions `perms` (user read and write by default).  The
     returned shared object `obj` is automatically detached when no longer
     referenced by Yorick.

     Argument `type` can be one of `TAO_SHARED_OBJECT`, `TAO_SHARED_ARRAY` or
     `TAO_SHARED_CAMERA`.

   SEE ALSO: tao_attach_shared_object.
 */

local TAO_INT8, TAO_UINT8, TAO_INT16, TAO_UINT16, TAO_INT32, TAO_UINT32,
    TAO_INT64, TAO_UINT64, TAO_FLOAT32, TAO_FLOAT64;
extern tao_attach_shared_array;
extern tao_attach_shared_camera;
extern tao_attach_shared_object;
/* DOCUMENT obj = tao_attach_shared_object(ident);
         or obj = tao_attach_shared_object(ident, type);
         or arr = tao_attach_shared_array(ident);
         or cam = tao_attach_shared_camera(ident);

     Function `tao_attach_shared_object` attaches the shared TAO object
     identified by `ident` to the address space of the Yorick process and
     returns it.  The returned shared object `obj` is automatically detached
     when no longer referenced by Yorick.  Optional argument `type` can be one
     of `TAO_SHARED_OBJECT`, `TAO_SHARED_ARRAY` or `TAO_SHARED_CAMERA` if a
     specific object type is expected (`TAO_SHARED_OBJECT` is the default).

     Function `tao_attach_shared_array` attaches the shared TAO array
     identified by `ident` to the address space of the Yorick process and
     returns it.  The returned shared object `arr` is automatically detached
     when no longer referenced by Yorick.

     Function `tao_attach_shared_camera` attaches the shared TAO camera
     identified by `ident` to the address space of the Yorick process and
     returns it.  The returned shared object `cam` is automatically detached
     when no longer referenced by Yorick.

     The returned results have the following members:

         obj.ident        identifier of shared object;
         obj.type         type of shared object;
         obj.size         size (in bytes) of shared object;

         arr.ident        identifier of shared array;
         arr.type         type of shared array (that is TAO_SHARED_ARRAY);
         arr.size         size (in bytes) of shared array;
         arr.eltype       type of elements of shared array;
         arr.ndims        number of dimensions of shared array;
         arr.dims         dimensions (as with `dimsof`) of shared array;
         arr.data         data (as a Yorick array) of shared array;
         arr.nreaders     number of readers on this array;
         arr.nwriters     number of writers on this array;
         arr.counter      counter of shared array;
         arr.timestamp    time-stamp of shared array (a pair of integers,
                          `[s,ns]`, with the number of seconds and nanoseconds);

         cam.ident        identifier of shared array;
         cam.type         type of shared array (that is TAO_SHARED_ARRAY);
         cam.size         size (in bytes) of shared array;
         cam.state        camera state (0 if device not yet open, 1 if device
                          open but no acquisition is running, 2 if acquisition
                          is running);
         cam.sensorwidth  detector horizontal size (in pixels);
         cam.sensorheight detector vertical size (in pixels);
         cam.depth        bits per pixel in the raw captured images;
         cam.eltype       type of elements of processed images;
         cam.xbin         horizontal binning (in pixels);
         cam.ybin         vertical binning (in pixels);
         cam.xoff         horizontal offset of ROI (in pixels);
         cam.yoff         vertical offset of ROI (in pixels);
         cam.width        number of macro-pixels per line in acquired images;
         cam.height       number of lines of macro-pixels in acquired images;
         cam.roi          region of interest: `[xoff,yoff,width,height]`;
         cam.framerate    number of frames per second;
         cam.exposuretime exposure duration (in seconds);

     The possible values of `eltype` are:

         -----------------------------------------
         Constant   Value  Description
         -----------------------------------------
         TAO_INT8      1   Signed 8-bit integer
         TAO_UINT8     2   Unsigned 8-bit integer
         TAO_INT16     3   Signed 16-bit integer
         TAO_UINT16    4   Unsigned 16-bit integer
         TAO_INT32     5   Signed 32-bit integer
         TAO_UINT32    6   Unsigned 32-bit integer
         TAO_INT64     7   Signed 64-bit integer
         TAO_UINT64    8   Unsigned 64-bit integer
         TAO_FLOAT32   9   32-bit floating-point
         TAO_FLOAT64  10   64-bit floating-point
         -----------------------------------------


   SEE ALSO: tao_create_shared_object, tao_create_shared_array,
             tao_get_data, tao_wait_image.
 */

extern tao_get_data;
extern tao_set_data;
/* DOCUMENT arr = tao_get_data(obj);
         or tao_set_data, obj, arr;

     The function `tao_get_data` yields the contents of a TAO shared array
     `obj` as a Yorick array.  Calling this function is equivalent to:

         obj.data

     The function `tao_set_data` copies the elements of Yorick array `arr`
     into TAO shared array `obj`.  The two arrays must have the same
     dimensions but may have different element types (they are automatically
     converted).  If called as a function, `tao_set_data` returns its first
     argument.


   SEE ALSO: tao_create_shared_array, tao_attach_shared_object.
 */

extern tao_get_counter;
extern tao_get_timestamp;
/* DOCUMENT cnt = tao_get_counter(arr);
         or tms = tao_set_timestamp(arr);

     The function `tao_get_counter` yields the value of the image counter
     of TAO shared array `arr`.

     The function `tao_get_timestamp` yields the value of the time-stamp of
     TAO shared array `arr`.  The returned time-stamp is a pair of two
     integers, the number of seconds and the number of nanoseconds elapsed
     since some unspecified epoch.

     These two functions are respectively equivalent to:

         arr.counter
         arr.timestamp


   SEE ALSO: tao_wait_image, tao_get_data, tao_get_monotonic_time.
 */

local TAO_SHARED_CAMERA_SEMAPHORES;
extern tao_wait_image;
/* DOCUMENT arr = tao_wait_image(cam, sem);
         or arr = tao_wait_image(cam, sem, secs);

     The function `tao_wait_image` waits on semaphore number `sem` for a new
     image to be available from TAO shared camera `cam`.  Optional argument
     `secs` is the maximum amount of time to wait (in seconds).  If no new
     image is acquired during the allowed time, an empty result is returned,
     otherwise a TAO shared array is returned with the image data.  If `secs`
     is unspecified (not recommended), the call blocks until a new image
     becomes available.

     It is assumed by the frame grabber that at most one process is waiting on
     each semaphore, the chosen semaphore number must thus be chosen in
     agreement with the choice made by other processes.  The first semaphore
     number is 1 and Yorick indexing rules are applied for the parameter `sem`
     (that is, `sem = 0` is the last semaphore, `sem = -1` is the penultimate
     one, etc.).  Global variable `TAO_SHARED_CAMERA_SEMAPHORES` gives the
     number of semaphores associated with a shared camera.

   SEE ALSO: tao_create_shared_array, tao_attach_shared_object.
 */

extern tao_lock;
extern tao_try_lock;
extern tao_unlock;
/* DOCUMENT tao_lock, obj;
         or ans = tao_try_lock(obj);
         or tao_unlock, obj;

     The function `tao_lock` locks object `obj`.  If the object is locked by
     another process, the call will block until the object is unlocked.

     The function `tao_try_lock` attempts to lock object `obj` without
     blocking.  If the object is locked by another process, `0` is returned;
     otherwise, the object `obj` becomes locked by the caller and `1` is
     returned.

     The function `tao_unlock` unlocks object `obj` which has been locked
     by the caller.

   SEE ALSO: tao_attach_shared_object.
 */

func tao_query_shmid(apt) { return _tao_query(long, apt, "shmid"); }
/* DOCUMENT shmid = tao_query_shmid(apt);

      Yields the shared memory identifier of data shared by TAO server whose
      XPA access point is APT.  Example:

          apt = "andorcam1";
          sem = 1;
          timeout = 5.0;
          cam = tao_attach_shared_camera(tao_query_shmid(apt));
          tao_config, apt, framerate=40, exposuretime=0.01;
          tao_start, apt;
          arr = tao_wait_image(cam, sem, timeout);
          if (! is_void(arr)) {
              fma;
              pli, arr.data
          }
          tao_stop;

   SEE ALSO: tao_attach_shared_object.
 */

func tao_query_xbin(apt) { return _tao_query(long, apt, "xbin"); }
func tao_query_ybin(apt) { return _tao_query(long, apt, "ybin"); }
/* DOCUMENT tao_query_xbin(apt)
         or tao_query_ybin(apt)

     Yield the horizontal and vertical binning (in physical pixels) of the
     images acquired by the camera server at XPA access point APT.

   SEE ALSO: tao_config.
 */

func tao_query_xoff(apt) { return _tao_query(long, apt, "xoff"); }
func tao_query_yoff(apt) { return _tao_query(long, apt, "yoff"); }
/* DOCUMENT tao_query_xoff(apt)
         or tao_query_yoff(apt)

     Yield the offsets (in physical pixels) of the region of interest relative
     to the sensor edges for the camera server at XPA access point APT.

   SEE ALSO: tao_config.
 */

func tao_query_width(apt) { return _tao_query(long, apt, "width"); }
func tao_query_height(apt) { return _tao_query(long, apt, "height"); }
/* DOCUMENT tao_query_width(apt)
         or tao_query_height(apt)

     Yield the horizontal and vertical dimensions (in macro-pixels) of the
     images acquired by the camera server at XPA access point APT.

   SEE ALSO: tao_config.
 */

func tao_query_sensorwidth(apt) { return _tao_query(long, apt, "sensorwidth"); }
func tao_query_sensorheight(apt) { return _tao_query(long, apt, "sensorheight"); }
/* DOCUMENT tao_query_sensorwidth(apt)
         or tao_query_sensorheight(apt)

     Yield the horizontal and vertical dimensions (in physical pixels) of the
     detector managed by the camera server at XPA access point APT.

   SEE ALSO: tao_config.
 */
func tao_query_exposuretime(apt) { return _tao_query(double, apt, "exposuretime"); }
func tao_query_framerate(apt) { return _tao_query(double, apt, "framerate"); }
/* DOCUMENT tao_query_exposuretime(apt)
         or tao_query_framerate(apt)

     Yield the exposure time (in seconds) and frame rate (in frame per
     seconds) settings for the acquisition by the camera server at XPA access
     point APT.

   SEE ALSO: tao_config.
 */
func tao_query_state(apt) { return _tao_query(string, apt, "state"); }
/* DOCUMENT tao_query_state(apt)

     Yield the current state of the acquisition by the camera server at XPA
     access point APT.

   SEE ALSO: tao_config.
 */

func tao_stop(apt) { xpa_set, apt, "stop"; }
func tao_abort(apt) { xpa_set, apt, "abort"; }
func tao_quit(apt) { xpa_set, apt, "quit"; }
func tao_start(apt, nbufs)
/* DOCUMENT tao_start, apt;
         or tao_start, apt, nbufs;
         or tao_stop, apt;
         or tao_abort, apt;
         or tao_quit, apt;

      Send an acquisition command to the XPA server identiifed by APT.
      Optional argument NBUFS is the number of acquisition buffers to use.

   SEE ALSO: tao_config, tao_query_state.
 */
{
    cmd = (is_void(nbufs) ? "start" : swrite(format="start %d", nbufs));
    xpa_set, apt, cmd;
}

func _tao_query(type, apt, key)
{
    ans = xpa_get(apt, key);
    replies = ans();
    if (replies != 1) {
        error, write(format="expecting one answer (got %d)", replies);
    }
    str = ans(1,4);
    return (type == string ? tao_chomp(str) : tao_parse(type, str));
}
errs2caller, _tao_query;

func tao_chomp(str)
/* DOCUMENT tao_chomp(str);

     Yields string STR without tariling newline if any.

   SEE ALSO: strtrim, strpart.
 */
{
    return (strpart(str, 0:0) == "\n" ? strpart(str, 1:-1) : str);
}

local _tao_esc;
func tao_config(args)
/* DOCUMENT tao_config, apt, key1=val1, key2=val2, ...;

     When used as a subroutine, sends an XPA set "config" command to the XPA
     server APT with settings given by the KEYn=VALn pairs.

   SEE ALSO: tao_query_shmid, tao_start, tao_query_xbin, tao_query_ybin,
             tao_query_xoff, tao_query_yoff, tao_query_width, tao_query_height,
             tao_query_sensorwidth, tao_query_sensorheight, tao_query_state.
 */
{
    nargs = args(*); // number of positional arguments
    if (nargs != 1) {
        error, "syntax: tao_config, apt, key1=val1, key2=val2, ...;";
    }
    apt = args(1);
    keys = args(-);
    nkeys = numberof(keys);
    if (nkeys < 1) return;
    cmd = "config";
    for (k = 1; k <= nkeys; ++k) {
        key = keys(k);
        val = args(key);
        type = structof(val);
        if (! is_scalar(val)) {
            error, swrite(format="non-scalar value for key \"%s\"", key);
        }
        if (type == long || type == int || type == short || type == char) {
            fmt = "%s %s %d";
        } else if (type == double || type == float) {
            fmt = "%s %s %g";
        } else if (type == string) {
            if (! strglob("*'*", val)) {
                /* no single quote */
                fmt = "%s %s '%s'";
            } else {
                /* string has at least one single quote, length > 0 in this
                   case */
                bytes = strchar(val)(1:-1);
                i = where(_tao_esc(bytes));
                if (is_array(i)) {
                    buf = array(char, 2, numberof(bytes));
                    buf(1,i) = '\\';
                    buf(2,) = bytes;
                    val = strchar(buf(where(buf)));
                }
                fmt = "%s %s \"%s\"";
            }
        } else {
            error, swrite(format="invalid value type for key \"%s\"", key);
        }
        cmd = swrite(format=fmt, cmd, key, val);
    }
    xpa_set, apt, cmd;
}
wrap_args, tao_config;

_tao_esc = array(0n, 255);
_tao_esc('\t') = 1n;
_tao_esc('\n') = 1n;
_tao_esc('\r') = 1n;
_tao_esc('\\') = 1n;
_tao_esc('"') = 1n;

func tao_parse(type, str)
/* DOCUMENT val = tao_parse(type, str);

      Yields a value of given type from string STR.  An error is thrown
      if STR cannot be converted to a value of the given type.

   SEE ALSO: sread.
 */
{
    value = type();
    dummy = string();
    if (sread(str, value, dummy)) {
        return value;
    }
    error, swrite(format="string \"%s\" is not a valid \"%s\" value", str,
                  nameof(type));
}
errs2caller, tao_parse;

extern _tao_init;
/* DOCUMENT _tao_init;

      Initializes constants of TAO plugin.  Can be called again to restore
      them.

   SEE ALSO: tao_attach_shared_object.
 */
_tao_init;
