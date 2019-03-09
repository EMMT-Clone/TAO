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

"""

```julia
Themis.calibrate(Themis.InteractionMatrix, ...) -> dat
```

yields the data needed to calibrate the interaction matrix of
the adaptive optics system.

"""
function calibrate(::Type{InteractionMatrix},
                   gain::Matrix{<:AbstractFloat},
                   bias::Matrix{<:AbstractFloat},
                   cam::Phoenix.Camera,
                   dm::Alpao.DeformableMirror,
                   cref::Vector{Cdouble},
                   xref::Vector{Cdouble},
                   yref::Vector{Cdouble},
                   num::Integer;
                   reset::Bool = false,
                   rho::NTuple{2,Real} = (2.5, 0.001),
                   amp::Real = 0.1,
                   nbufs::Integer = 8,
                   skip::Integer = 5,
                   timeout::Real = defaulttimeout(cam),
                   truncate::Bool = false,
                   drop::Bool = true)

    # FIXME: hardcoded pixel type!
    T = UInt8

    # Check arguments.
    num ≥ 1 || throw(ArgumentError("invalid number of images"))
    skip ≥ 1 || throw(ArgumentError("invalid number of images to skip"))
    timeout > zero(timeout) || throw(ArgumentError("invalid timeout"))

    nsubs = length(xref)
    ncmds = length(dm)
    @assert length(yref) == nsubs

    x = Array{Cdouble}(undef, nsubs)
    y = Array{Cdouble}(undef, nsubs)
    cmds = Array{Cdouble}(undef, ncmds, num)
    data = Array{Cdouble}(undef, 2, nsubs, num)

    # Set of values for the binomial random distribution.
    s = [Cdouble(-amp), Cdouble(+amp)]

    # Start acquisition with given callback and collect images.
    cnt = 0
    send!(dm, cref)
    start(cam, T, nbufs)
    prev = zeros(Cdouble, ncmds)
    while cnt < num
        try
            img, ticks = wait(cam, timeout, drop)
            cmd = rand(s, ncmds)
            send!(dm, cmd .+ cref) # FIXME: Deal with clipping!
            if skip > zero(skip)
                # Skip this frame.
                skip -= one(skip)
                release(cam)
            else
                # Process this frame.
                cnt += 1
                if cnt == 1 || reset
                    copyto!(x, xref)
                    copyto!(y, yref)
                else
                    # FIXME: optimize
                    copyto!(x, data[1,:,cnt-1])
                    copyto!(y, data[2,:,cnt-1])
                end
                fitcentroids!(img, gain, bias, x, y; rho=rho)
                release(cam)
                cmds[:,cnt] .= prev
                data[1,:,cnt] .= x
                data[2,:,cnt] .= y
            end
            copyto!(prev, cmd)
        catch err
            if truncate && isa(err, TimeoutError)
                @warn "Acquisition timeout after $cnt image(s)"
                if cnt < num
                    if cnt > 0
                        cmds = cmds[:,1:cnt]
                        data = data[:,:,1:cnt]
                    else
                        cmds = Array{Cdouble}(undef, ncmds, 0)
                        data = Array{Cdouble}(undef, 2, nsubs, 0)
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
    return (cmds, data)
end
