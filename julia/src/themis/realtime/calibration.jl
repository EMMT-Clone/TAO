#
# themis/realtime/calibration.jl --
#
# Calibration methods for Themis adaptive optics system using the
# devices (deformable mirror, wavefront sensor camera, etc.).
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut & Michel Tallon.
#

identity(x) = x

"""

```julia
ThemisRealTime.calibrate(Themis.InteractionMatrix, cam, dm, cref, xyref,
                         preprocess = identity) -> (U, V)
```

yields the data needed to calibrate the interaction matrix of the adaptive
optics system.  Arguments are the wavefront sensor camera `cam`, the deformable
mirror `dm`, the reference command `cref` to apply to the deformable mirror to
compensate for static aberrations, the corresponding reference positions of the
spots `xyref` on the wavefront sensor image.  Optional arguemtn `preprocess` is
the preprocessing method to apply to the images delivered by the wavefront
sensor camera `cam` (by default, no preprocessing is applied).

The returned result is a tuple `(U,V)` where `U` is an array of dimensions
`(nacts,nimgs)` storing the commands sent to the deformable mirror and `V` is
an array of dimensions `(2,nsubs,nimgs)` with the coordinates of the positions
given by the wacefront sensor.  Here `ncdms` is the number of actuators of the
deformable mirror, `nsubs` is the number of active wavefront sensor
sub-apertures and `nimgs` is the effective number of images in the sequence.

Possible keywords are:

- Keyword `nimgs` specifies the number of wavefront sensor images to acquire
  (default: `1000`).

- If keyword `absolute` is true, the absolute commands and the absolute spot
  positions are returned in `U` and `V`; otherwise, the returned commands and
  positions are relative to the reference ones (default: `false`).

- If keyword `reset` is true, the search of the spots is started near the
  reference positions, otherwise, the search is started near the previous
  positions (default: `true`).

- Keyword `amp` specifies the amplitude of the random commands sent to the
  deformable mirror to calibrate the interaction matrix (default: `0.1`).

- Keyword `nbufs` specifies the number of image buffers to use for acquisition
  (default: `8`).

- Keyword `skip` specifies the number of initial frames to skip (default: `5`).
  This is useful to avoid initial garbage images for cameras without a shutter.

- Keyword `timeout` specifies the maximum number of seconds to wait for the
  next image from the wavefront sensor camera (default: `defaulttimeout(cam)`).

- Keyword `truncate` specifies whether to accept a truncated sequence of data
  (default: `false`).

- Keyword `drop` specifies whether to drop frames older than the last one
  (default: `true`)

- Keyword `rho` (default: `(2.5, 0.001)`).

Other keywords are passed to `Centroiding.fit!`.

"""
function calibrate(::Type{<:InteractionMatrix},
                   cam::Phoenix.Camera,
                   dm::Alpao.DeformableMirror,
                   cref::Vector{Cdouble},
                   xyref::Matrix{Cdouble},
                   preprocess::Function = identity;
                   nimgs::Integer = 1000,
                   absolute::Bool = false,
                   reset::Bool = true,
                   amp::Real = 0.1,
                   nbufs::Integer = 8,
                   skip::Integer = 5,
                   timeout::Real = Phoenix.defaulttimeout(cam),
                   truncate::Bool = false,
                   drop::Bool = true,
                   rho::NTuple{2,Real} = (2.5, 0.001),
                   kwds...)

    # Check arguments.
    nimgs ≥ 1 || throw(ArgumentError("invalid number of images"))
    skip ≥ 1 || throw(ArgumentError("invalid number of images to skip"))
    timeout > zero(timeout) || throw(ArgumentError("invalid timeout"))
    @assert size(xyref, 1) == 2
    nsubs = size(xyref, 2)
    nacts = length(dm)
    @assert length(cref) == nacts

    # allocate arrays.
    xy = Array{Cdouble}(undef, 2, nsubs)
    U = Array{Cdouble}(undef, nacts, nimgs)
    V = Array{Cdouble}(undef, 2, nsubs, nimgs)

    # Set of values for the binomial random distribution.
    s = [Cdouble(-amp), Cdouble(+amp)]

    # Start acquisition with given callback and collect images.
    cnt = 0
    send!(dm, cref)
    start(cam, UInt8, nbufs)# FIXME: hardcoded pixel type!
    prev = zeros(Cdouble, nacts)
    while cnt < nimgs
        try
            raw, ticks = wait(cam, timeout, drop)
            cmd = rand(s, nacts)
            send!(dm, cmd .+ cref) # FIXME: Deal with clipping!
            if skip > zero(skip)
                # Skip this frame.
                skip -= one(skip)
                release(cam)
            else
                # Process this frame.
                cnt += 1
                if cnt == 1 || reset
                    copyto!(xy, xyref)
                end
                img = preprocess(raw)
                release(cam)
                Centroiding.fit!(img, xy; rho=rho, kwds...)
                if absolute
                    @inbounds @simd for j in 1:nacts
                        U[j,cnt] = prev[j] + cref[j]
                    end
                    @inbounds @simd for i in 1:nsubs
                        V[1,i,cnt] = xy[1,i]
                        V[2,i,cnt] = xy[2,i]
                    end
                else
                    @inbounds @simd for j in 1:nacts
                        U[j,cnt] = prev[j]
                    end
                    @inbounds @simd for i in 1:nsubs
                        V[1,i,cnt] = xy[1,i] - xyref[1,i]
                        V[2,i,cnt] = xy[2,i] - xyref[2,i]
                    end
                end
            end
            copyto!(prev, cmd)
        catch err
            if truncate
                if isa(err, TimeoutError)
                    msg = "Acquisition timeout"
                else
                    msg = string(err)
                end
                @warn "Acquisition stopped after $cnt image(s) [$msg]"
                if cnt < nimgs
                    if cnt > 0
                        U = U[:,1:cnt]
                        V = V[:,:,1:cnt]
                    else
                        U = Array{Cdouble}(undef, nacts, 0)
                        V = Array{Cdouble}(undef, 2, nsubs, 0)
                    end
                end
            else
                abort(cam)
                rethrow(err)
            end
        end
    end

    # Stop immediately.
    abort(cam)

    # Return results.
    return (U, V)
end
