## Things to do

* Add anonymous semaphores to signal new images.

* Instead of immediately attempt to reflect the configuration of the camera
  after any parameter change, it may be better to defer that until starting the
  acquisition.  Optionally a function, say `tao_apply_camera_configuration`,
  may be called before to trigger device configuration before.  Other
  functions, say `tao_check_camera_configuration` and
  `tao_reset_camera_configuration`, may be called to check the current settings
  (before applying them) or to revert to actual camera settings.

* Specialize `attach(::Type{TAO.SharedArray{T}},id)` and
  `attach(::Type{TAO.SharedArray{T,N}},id)` to have a more specific returned
  value.

* Extract error messages in Julia interface.

* Manage to have '-Wall -Werror -O2' the default compilation flags.


## Vectorization and real-time computations

* Make sure shared array values are optimally aligned to benefit from
  vectorization.  [in principle: OK]

* Compile several versions of the same critical functions (e.g. image
  preprocessing) with different SIMD options and choose the fastest one at
  runtime (like https://liboil.freedesktop.org/wiki/).

* Add stride term in `tao_preprocess_image_*` methods.  Manage to compile this
  code with vectorization.

* It may be faster (cf., CPng camera) to process the raw image line by line
  (possibly using a few threads):

  - extract/unpack one line of raw pixels in a suitably aligned buffer
    (perhaps do type conversion at this moment);
  - pre-process the pixels from the aligned buffer;
  - move to next line;

  This would also solve some issues like having strided raw images (e.g.,
  Andor camera or when ROI is smaller than the captured image because of
  constraints), having packed pixel values in the captured image, ...


## Integer type for array indices

* What is the best integer type for array indices?  `ptrdiff_t` seems to be the
  right answer (See
  https://stackoverflow.com/questions/3174850/what-is-the-correct-type-for-array-indexes-in-c) but is it the fastest?

* I generally find more useful to use signed integer types even for nonnegative
  quantities because this avoids any unexpected result when doing arithmetic.


## Some benchmark results

```julia
using TAO
TAO.loadclib()
cam = attach(TAO.SharedCamera, 13533202)
TAO.LibTAO.get_last_image_counter(cam)
TAO.LibTAO.get_last_image_ident(cam)
using BenchmarkTools
@benchmark TAO.LibTAO.get_last_image_counter(cam) # takes ~ 11.2 ns/call
@benchmark TAO.LibTAO.get_last_image_ident(cam)   # takes ~ 11.2 ns/call
```

as a comparison:

```julia
unsafe_get_state(cam::TAO.SharedCamera)::Cint =
    unsafe_load(Ptr{Cint}(cam.ptr + 64))
unsafe_set_state!(cam::TAO.SharedCamera,val::Integer) =
    unsafe_store!(Ptr{Cint}(cam.ptr + 64), val)
get_state(cam::TAO.SharedCamera)::Cint =
    (cam.ptr == C_NULL ? -1 : unsafe_load(Ptr{Cint}(cam.ptr + 64)))
set_state!(cam::TAO.SharedCamera,val::Integer) =
    cam.ptr != C_NULL && unsafe_store!(Ptr{Cint}(cam.ptr + 64), val)
@benchmark unsafe_get_state(cam)     # takes ~  3.4 ns/call
@benchmark get_state(cam)            # takes ~  3.9 ns/call
@benchmark unsafe_set_state!(cam, 3) # takes ~  5.2 ns/call
@benchmark set_state!(cam, 3)        # takes ~ 11.2 ns/call
```

same functions but with `@noinline`:

```julia
@noinline unsafe_get_state(cam::TAO.SharedCamera)::Cint =
    unsafe_load(Ptr{Cint}(cam.ptr + 64))
@noinline unsafe_set_state!(cam::TAO.SharedCamera,val::Integer) =
    unsafe_store!(Ptr{Cint}(cam.ptr + 64), val)
@noinline get_state(cam::TAO.SharedCamera)::Cint =
    (cam.ptr == C_NULL ? -1 : unsafe_load(Ptr{Cint}(cam.ptr + 64)))
@noinline set_state!(cam::TAO.SharedCamera,val::Integer) =
    cam.ptr != C_NULL && unsafe_store!(Ptr{Cint}(cam.ptr + 64), val)
@benchmark unsafe_get_state(cam)     # takes ~  4.5 ns/call
@benchmark get_state(cam)            # takes ~  4.5 ns/call
@benchmark unsafe_set_state!(cam, 3) # takes ~  6.0 ns/call
@benchmark set_state!(cam, 3)        # takes ~ 12.5 ns/call
```

Thus the extra time taken to check argument is not significant when getters are
used to retrieve members of shared structures.  As a matter of fact, the
timings of the new getters (which accept a `NULL` pointer and use branch
prediction) are the same as before: about 11.2 ns/call on the same machine.

With the new version of the callers (which check the validity of the obejct
address and after removed that checking from the
`unsafe_convert(::Type{Ptr{T}},obj)` method) the new timings are impressive:

```julia
using TAO
TAO.loadclib()
cam = attach(TAO.SharedCamera, 13533202)
cam0 = TAO.SharedCamera()  # has a NULL pointer
TAO.LibTAO.get_last_image_counter(cam)
TAO.LibTAO.get_last_image_ident(cam)
using BenchmarkTools
@benchmark TAO.LibTAO.get_last_image_counter(cam)  # takes ~ 4.5 ns/call
@benchmark TAO.LibTAO.get_gain(cam)                # takes ~ 4.2 ns/call
@benchmark TAO.LibTAO.get_last_image_counter(cam0) # takes ~ 5.0 ns/call
@benchmark TAO.LibTAO.get_gain(cam0)               # takes ~ 5.0 ns/call
```

Thus these versions are much faster (perhaps the Julia check can be made faster
by making the `unsafe_convert` method inlined of by having it call another
method in case of error instead of embedding the throwing of the error).  Note
that branch prediction saves about 0.5 ns/call.

Specifying argument types via `Union{...}` has a cost!  For instance:

```julia
get_type(obj::AnySharedObject) =
   ccall((:tao_get_shared_object_type, taolib), Cint, (Ptr{Cvoid},), obj)
```

takes ~ 20 ns/call, while:

```julia
get_type(obj::SharedCamera) =
   ccall((:tao_get_shared_object_type, taolib), Cint, (Ptr{Cvoid},), obj)
```

takes ~ 4.5 ns/call.

Other benchmarks:

* Calling `lock(obj)` then `unlock(obj)` takes ~ 60 ns.

* Attach + detach of a shared memory segment takes a few µs (3.7µs on average
  on my laptop).

* The `ping` command takes about 2.5 milliseconds with the shell command
  `xpaget` but only 400 µs when using a persistent connection.
