#
# detector.jl --
#
# Calibration of the image detector and precprocessing of raw acquired images.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

module Detector

import ...TAO: bcastcopy, bcastdims, bcastlazy, WeightedArray

# FIXME: add fitting of the smooth model

struct Calibration{T<:AbstractFloat,N,
                   Ta<:DenseArray{T,N},
                   Tb<:DenseArray{T,N},
                   Tu<:DenseArray{T,N},
                   Tv<:DenseArray{T,N},
                   Tg<:DenseArray{T,N},
                   Ts<:DenseArray{T,N}}
    # Amplitude correction factor (in flux units per digital units):
    a::Ta

    # Detector bias plus mean background (in digital units):
    b::Tb

    # Variance parameters:
    u::Tu
    v::Tv

    # Detector gain (in electrons per digital units):
    gain::Tg

    # Standard deviation of the background noise (in digital units):
    std::Ts
end

"""

```julia
TAO.Detector.calibrate(md, vd, ms, vs, mf = ms) -> cal
```

yields an instance of [`TAO.Detector.Calibration`](@ref) which can be used to
apply pre-processing of raw images acquired by a detector.  The arguments are
arrays of compatible sizes (see [`broadcast`](@ref)):

 * `md` and `vd` are the empirical mean and variance of a series of *dark*
   images that is raw images acquired with no illumination.

 * `ms` and `vs` are the empirical mean and variance of a series of
   images; that is, raw images acquired with some *stable* illumination.

 * Optionally, `mf` is a mean *flat* image.  If no specified `ms` is used
   instead.

All images are assumed to have been acquired under the same conditions.  When
variances are needed, the corresponding series of raw images must be acquired
under stable conditions (otherwise the empirical variance also account for the
variance of the instabilities).

Providing a *flat* image (different from `ms`) is meant to also compensate for
nonuniform transmission of the optics.  If `mf` is not supplied, `ms` should
correspond to a uniform illumination.

"""
function calibrate(md, vd, ms, vs, mf; kwds...)
    T = float(promote_type(eltype(md), eltype(vd),
                           eltype(ms), eltype(vs),
                           eltype(mf)))
    dims = bcastdims(size(md), size(vd),
                     size(ms), size(vs),
                     size(mf))
    return calibrate(bcastlazy(T, dims, md), bcastlazy(T, dims, vd),
                     bcastlazy(T, dims, ms), bcastlazy(T, dims, vs),
                     bcastlazy(T, dims, mf); kdws...)
end

function calibrate(md, vd, ms, vs; kwds...)
    T = float(promote_type(eltype(md), eltype(vd),
                           eltype(ms), eltype(vs)))
    dims = bcastdims(size(md), size(vd),
                     size(ms), size(vs))
    return calibrate(bcastlazy(T, dims, md), bcastlazy(T, dims, vd),
                     bcastlazy(T, dims, ms), bcastlazy(T, dims, vs); kdws...)
end

function calibrate(md::DenseArray{T,N},
                   vd::DenseArray{T,N},
                   ms::DenseArray{T,N},
                   vs::DenseArray{T,N},
                   mf::DenseArray{T,N} = ms;
                   kwds...) where {T<:AbstractFloat,N}
    dims = size(md)
    @assert size(vd) == dims
    @assert size(ms) == dims
    @assert size(vs) == dims
    @assert size(mf) == dims

    a    = Array{T}(undef, dims)
    b    = Array{T}(undef, dims)
    u    = Array{T}(undef, dims)
    v    = Array{T}(undef, dims)
    gain = Array{T}(undef, dims)
    std  = Array{T}(undef, dims)
    tr   = Array{T}(undef, dims)

    minvar = zero(T)
    mdl = one(T)
    vdef = T(1.0) # to avoid division by zero

    @inbounds @simd for i in eachindex(a, md, vd, ms, vs, mf)
        # a = mdl/(mf - md)
        if isfinite(mf[i]) && isfinite(md[i]) && mf[i] > md[i]
            # FIXME: add a check about the flat model
            a[i] = mdl/(mf[i] - md[i])
        else
            a[i] = 0
        end

        # b = md
        if isfinite(md[i])
            b[i] = md[i]
        else
            b[i] = 0
        end

        # std = sqrt(vd)
        if isfinite(vd[i]) && vd[i] > minvar
            std[i] = sqrt(vd[i])
        else
            std[i] = 0
        end

        # gain = (ms - md)/(vs - vd)
        if (isfinite(ms[i]) && isfinite(md[i]) && ms[i] > md[i] &&
            isfinite(vs[i]) && isfinite(vd[i]) && vs[i] > vd[i])
            gain[i] = (ms[i] - md[i])/(vs[i] - vd[i])
        else
            gain[i] = 0
        end

        # u = gain/a
        if a[i] > 0 && gain[i] > 0
            u[i] = gain[i]/a[i]
        else
            u[i] = 0
        end

        # v = a*gain*vd
        if u[i] > 0 && std[i] > 0
            v[i] = a[i]*gain[i]*vd[i]
        else
            v[i] = vdef
        end
    end

    return Calibration(a, b, u, v, gain, std)
end

"""

```julia
TAO.Detector.process(TAO.WeightedArray, cal, raw) -> wgtimg
```

yields an instance of [`TAO.WeightedArray`](@ref) which is obtained by
pre-processing the image `raw` acquired by the detector whose calibration
parameters are given by `cal` (an instance of
[`TAO.Detector.Calibration`](@ref)).

The operation can be applied in-place:

```julia
TAO.Detector.process(wgtimg, cal, raw) -> wgtimg
```

to overwrite the contents of `wgtimg` by the result of the pre-processing.

See also: [`TAO.Detector.calibrate`](@ref).

"""
function process(::Type{WeightedArray},
                 cal::Calibration{Tc,N},
                 raw::DenseArray{Tr,N}) where {Tc,Tr,N}
    Td = float(promote_type(Tc,Tr))
    return process(WeightedArray{Td,N}, cal, raw)
end

function process(::Type{WeightedArray{Td,N}},
                 cal::Calibration{Tc,N},
                 raw::DenseArray{Tr,N}) where {Td,Tc,Tr,N}
    dims = size(raw)
    dst = WeightedArray(Array{Td}(undef, dims), Array{Td}(undef, dims))
    return process!(dst, cal, raw)
end

function process!(dst::WeightedArray{Td,N},
                  cal::Calibration{Tc,N},
                  raw::DenseArray{Tr,N}) where {Td,Tc,Tr,N}
    dims = size(raw)
    wgt, dat = dst.wgt, dst.dat
    @assert size(wgt) == dims
    @assert size(dat) == dims
    a, b, u, v = cal.a, cal.b, cal.u, cal.v
    @assert size(a) == dims
    @assert size(b) == dims
    @assert size(u) == dims
    @assert size(v) == dims
    @inbounds @simd for i in eachindex(wgt, dat, raw, a, b, u, v)
        dat[i] = a[i]*(raw[i] - b[i])
        wgt[i] = u[i]/(max(dat[i], 0) + v[i])
    end
    return dst
end

@doc @doc(process) process!

end # module
