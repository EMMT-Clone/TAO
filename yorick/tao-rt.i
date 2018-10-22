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

extern tao_test;
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

extern tao_attach_shared_object;
/* DOCUMENT obj = tao_attach_shared_object(ident);
         or obj = tao_attach_shared_object(ident, type);

     Attachs the shared TAO object identified by `ident` to the address space
     of the Yorick process and returns it.  The returned shared object `obj`
     is automatically detached when no longer referenced by Yorick.

     Optional argument `type` can be one of `TAO_SHARED_OBJECT`,
     `TAO_SHARED_ARRAY` or `TAO_SHARED_CAMERA` if a specific object type is
     expected (`TAO_SHARED_OBJECT` is the default).

   SEE ALSO: tao_create_shared_object.
 */

extern tao_lock;
extern tao_try_lock;
extern tao_unlock;
/* DOCUMENT tao_lock, obj;
         or ans = tao_try_lock(obj);
         or tao_unlock, obj;

     The function `tao_lock` locks object `obj`.  If the object is locked by
     another process, the call will block * until the object is unlocked.

     The function `tao_try_lock` attempts to lock object `obj` without
     blocking.  If the object is locked by another process, `0` is returned;
     otherwise, the object `obj` becomes locked by the caller and `1` is
     returned.

     The function `tao_unlock` unlocks object `obj` which has been locked
     by the caller.

   SEE ALSO: tao_attach_shared_object.
 */

extern _tao_init;
_tao_init;
