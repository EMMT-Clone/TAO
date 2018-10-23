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
 * Copyright (C) 2018, Éric Thiébaut.
 */

if (is_func(plug_in)) plug_in, "tao_rt";

extern tao_get_current_time;
extern tao_get_monotonic_time;
/* DOCUMENT t = tao_get_current_time();
         or t = tao_get_monotonic_time();

     These function yields the ime with nanossecond resolution.  The returned
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

         obj.ident      identifier of shared object;
         obj.type       type of shared object;
         obj.size       size (in bytes) of shared object;

         arr.ident      identifier of shared array;
         arr.type       type of shared array (that is TAO_SHARED_ARRAY);
         arr.size       size (in bytes) of shared array;
         arr.eltype     type of elements of shared array;
         arr.ndims      number of dimensions of shared array;
         arr.dims       dimensions (as with `dimsof`) of shared array;
         arr.data       data (as a Yorick array) of shared array;
         arr.nreaders   number of readers on this array;
         arr.nwriters   number of writers on this array;
         arr.counter    counter of shared array;
         arr.timestamp  time-stamp of shared array (a pair of integers,
                        `[s,ns]`, with the number of seconds and nanoseconds);

         cam.ident      identifier of shared array;
         cam.type       type of shared array (that is TAO_SHARED_ARRAY);
         cam.size       size (in bytes) of shared array;
         cam.state      camera state (0 if device not yet open, 1 if device
                        open but no acquisition is running, 2 if acquisition
                        is running);
         cam.bias       detector bias;
         cam.gain       detector gain;
         cam.gamma      gamma correction factor;
         cam.fullwidth  detector horizontal size (in pixels);
         cam.fullheight detector vertical size (in pixels);
         cam.depth      bits per pixel in the raw captured images;
         cam.eltype     type of elements of processed images;
         cam.xoff       horizontal offset of ROI (in pixels);
         cam.yoff       vertical offset of ROI (in pixels);
         cam.width      number of macro-pixels per line in acquired images;
         cam.height     number of lines of macro-pixels in acquired images;
         cam.roi        region of interest: `[xoff,yoff,width,height]`;
         cam.rate       number of frames per second;
         cam.exposure   exposure duration (in seconds);

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

extern _tao_init;
/* DOCUMENT _tao_init;

      Initializes constants of TAO plugin.  Can be called again to restore
      them.

   SEE ALSO: tao_attach_shared_object.
 */
_tao_init;
