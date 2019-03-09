#
# weighted.jl --
#
# Implementation of weighted arrays.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

function WeightedArray(wgt::W, dat::D,
                       verify::Bool=false)::WeightedArray{T,N,W,D} where {
                           W<:DenseArray{T,N},D<:DenseArray{T,N}} where {
                               T<:AbstractFloat,N}
    if verify
        @inbounds for i in eachindex(wgt)
            w = wgt[i]
            if ! isfinite(w) || w < 0
                throw(ArgumentError("weights must be finite and nonnegative"))
            end
        end
        @inbounds for i in eachindex(dat)
            if ! isfinite(dat[i])
                throw(ArgumentError("data must be finite"))
            end
        end
    end
    return WeightedArray{T,N,W,D}(wgt, dat)
end

Base.ndims(A::WeightedArray{T,N}) where {T,N} = N
Base.eltype(A::WeightedArray{T,N}) where {T,N} = T
Base.length(A::WeightedArray) = length(data(A))
Base.size(A::WeightedArray) = size(data(A))
Base.size(A::WeightedArray, d) = size(data(A), d)
Base.axes(A::WeightedArray) = axes(data(A))
Base.axes(A::WeightedArray, d) = axes(data(A), d)

weights(A::WeightedArray) = A.wgt
data(A::WeightedArray) = A.dat

"""

```julia
fixweigtheddata(wgt, dat) -> fixwgt, fixdat
```

yields weights `fixwgt` and data `fixdat` whose values have been computed from
the weights `wgt` and the data `dat` so that for each index `i`:

  *`fixwgt[i] = wgt[i]` and `fixdat[i] = dat[i]` if `dat[i]` is finite and
    `wgt[i]` is finite and nonnegative;

  * `fixwgt[i] = 0` and `fixdat[i] = 0` otherwise.


The operation can be done in-place:

```julia
fixweigtheddata!(wgt, dat) -> wgt, dat
```

or by providing the resulting arrays (which can be the same as the input
arrays):

```julia
fixweigtheddata!(fixwgt, fixdat, wgt, dat) -> fixwgt, fixdat
```

The pair of arrays `(wgt,dat)` and `(fixwgt,fixdat)` can be repaced by an
insatnce of [`WeightedArray`](@ref).

"""
function fixweigtheddata(wgt::AbstractArray{T,N},
                         dat::AbstractArray{T,N}) where {T<:AbstractFloat,N}
    return fixweigtheddata!(similar(Array{T,N}, axes(wgt)),
                            similar(Array{T,N}, axes(dat)),
                            wgt, dat)
end

fixweigtheddata(A::WeightedArray) =
    WeightedArray(fixweigtheddata(weights(A), data(A))...)

function fixweigtheddata!(wgt::AbstractArray{T,N},
                          dat::AbstractArray{T,N}) where {T<:AbstractFloat,N}
    return fixweigtheddata!(wgt, dat, wgt, dat)
end

function fixweigtheddata!(fixwgt::AbstractArray{T,N},
                          fixdat::AbstractArray{T,N},
                          wgt::AbstractArray{T,N},
                          dat::AbstractArray{T,N}) where {T<:AbstractFloat,N}
    axes(fixwgt) == axes(fixdat) == axes(wgt) == axes(dat) ||
        throw(DimensionMismatch("all arrays must have the same indices"))
    @inbounds for i in eachindex(fixwgt, fixdat, wgt, dat)
        w, d = wgt[i], dat[i]
        if isfinite(d) && isfinite(w) && w ≥ 0
            fixwgt[i] = w
            fixdat[i] = d
        else
            fixwgt[i] = 0
            fixdat[i] = 0
        end
    end
    return (fixwgt, fixdat)
end

function fixweigtheddata!(A::WeightedArray)
    fixweigtheddata!(weights(A), data(A))
    return A
end

function fixweigtheddata!(dst::WeightedArray, A::WeightedArray)
    fixweigtheddata!(weights(dst), data(dst), weights(A), data(A))
    return dst
end

@doc @doc(fixweigtheddata) fixweigtheddata!
