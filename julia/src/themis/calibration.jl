#
# calibration.jl -
#
# Methods for the calibration of the Themis adaptive optics system.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut and Michel Tallon.
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

"""

```julia
Themis.fit(Themis.InteractionMatrix, U, V) -> G
```

yields the interaction matrix of the adaptive optics system given the
calibration data `U` and `V`.

Argument `U` is a sequence of known perturbations applied to the deformable
mirror.  More specifically `U[i,k]` is the command sent to the `i`-th actuator
during the `k`-th frame.  The perturbations must be random variables which are
centered and mutually independent.  If keyword `theoretical` is set true, the
perturbations must also be identically distributed.

Argument `V` is the corresponding sequence of wavefront sensor measurements.
More specifically `V[1,j,k]` and `V[2,j,k]` are the coordinates of the measured
slope in the `j`-th sub-aperture of the wavefront sensor during the `k`-th
frame.

Keyword `recenter` (`true` by default) specifies whether to subtract their time
average values to the measured wavefront slopes.

Keyword `theoretical` (`false` by default) specifies whether to use the
theoritical covariance of the random perturbations.  The theoritical covariance
matrix is assumed to be is proportional to the identity, hence the
perturbations shall be centered and i.i.d. (independent and identically
distributed).  The value of the variance is estimated from the sequence `U` of
perturbations unless keyword `stdev` is used to specify the standard deviation
of the perturbations.

"""
function fit(::Type{InteractionMatrix},
             U::AbstractArray{<:AbstractFloat,2},
             V::AbstractArray{<:AbstractFloat,3};
             recenter::Bool=true,
             sync::Bool=false,
             theoretical::Bool=false,
             stdev::Union{Nothing,Real}=nothing)
    if sync
        error("synchronization not yet implemented")
    end

    ncmds, ntimes = size(U)
    nsubs = size(V,2)
    @assert size(V) == (2, nsubs, ntimes)
    if recenter
        # Recenter the slopes V by subtracting their time average.
        V = V .- mean(V; dims=3)
    end
    # FIXME: Use generalized matrix product in LazyAlgebra (lgemm)
    Gx = V[1,:,:]*U'
    Gy = V[2,:,:]*U'
    if theoretical
        # Fit theoretical covariance distributution.
        if stdev === nothing
            a = sum(U.*U)/ncmds
        else
            a = stdev^2
        end
        @. Gx *= (1/a)
        @. Gy *= (1/a)
    else
        A = cholesky!(U*U')
        rdiv!(Gx, A)
        rdiv!(Gy, A)
    end

    # FIXME: use hcat/vcat?
    G = Array{eltype(Gx),3}(undef, 2, nsubs, ncmds)
    G[1,:,:] = Gx
    G[2,:,:] = Gy
    return G
end
