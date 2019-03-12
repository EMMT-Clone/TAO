#
# themis/calibration.jl -
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
average values to the commands and measured wavefront slopes.

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

    nacts, ntimes = size(U)
    nsubs = size(V,2)
    @assert size(V) == (2, nsubs, ntimes)
    if recenter
        # Recenter the slopes V by subtracting their time average.
        U = U .- mean(U; dims=2)
        V = V .- mean(V; dims=3)
    end
    # FIXME: Use generalized matrix product in LazyAlgebra (lgemm)
    Gx = V[1,:,:]*U'
    Gy = V[2,:,:]*U'
    if theoretical
        # Fit theoretical covariance distributution.
        if stdev === nothing
            a = sum(U.*U)/nacts
        else
            a = stdev^2
        end
        @. Gx *= (1/a)
        @. Gy *= (1/a)
    else
        # A = cholesky!(U*U')
        # FIXME: rdiv!(Gx, A)
        # FIXME: rdiv!(Gy, A)
        A = U*U'
        Gx = Gx/A
        Gy = Gy/A
    end

    # FIXME: use hcat/vcat?
    G = Array{eltype(Gx),3}(undef, 2, nsubs, nacts)
    G[1,:,:] = Gx
    G[2,:,:] = Gy
    return G
end
