#
# utils.jl --
#
# Utilities and miscellaneous methods for Julia package TAO (a Toolkit for
# Adaptive Optics software).
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

function TimeStamp(secs::AbstractFloat)
    ts_sec = floor(Int64, secs)
    ts_nsec = round(Int64, (secs - ts_sec)*1E9)
    return TimeStamp(ts_sec, ts_nsec)
end

TimeStamp(secs::Integer) = TimeStamp(secs, 0)

#------------------------------------------------------------------------------
# BROADCASTING

"""

`bcastcopy(T, dims, A)` yields a new array of type `T` and dimensions `dims`
filled with the elements of `A` according to type conversion and broadcasting
rules (see [`broadcast`](@ref)).  Compared to [`bcastlazy`](@ref), it is
guaranteed that the returned array does not share its contents with `A`.

"""
function bcastcopy(::Type{T}, dims::NTuple{N,Int}, A) where {T,N}
    arr = Array{T}(undef, dims)
    @. arr = A
    return arr
end

"""

`bcastlazy(T, dims, A)` yields a *dense* array of type `T` and dimensions
`dims` filled with the elements of `A` according to type conversion and
broadcasting rules (see [`broadcast`](@ref)).  Compared to [`bcastcopy`](@ref),
making a copy of `A` is avoided if it already has the correct type and
dimensions.  This means that the result may share the same contents as `A`.

"""
function bcastlazy(::Type{T}, dims::NTuple{N,Int},
                   A::DenseArray{T,N}) where {T,N}
    Base.has_offset_axes(A) &&
        throw(DimensionMismatch("array must have 1-based indices"))
    size(A) == dims ||
        throw(DimensionMismatch("array has invalid dimensions"))
    return A
end

bcastlazy(::Type{T}, dims::NTuple{N,Int}, A) where {T,N} =
    bcastcopy(T, dims, A)

"""

`bcastdims(size(A), size(B), ...)` yields the dimensions of the array that
would result from applying broadcasting rules (see [`broadcast`](@ref)) to
arguments `A`, `B`, etc.

"""
function bcastdims(A::NTuple{Na,Int}, B::NTuple{Nb,Int}) where {Na,Nb}
    @noinline baddim(len::Integer) = error("invalid array dimension $len")
    @noinline baddims(len1::Integer, len2::Integer) =
        error("incompatible array dimensions $len1 and $len2")
    if Na > Nb
        Nmin = Nb
        Nmax = Na
        A, B = B, A
    else
        Nmin = Na
        Nmax = Nb
    end
    dims = Array{Int}(undef, Nmax)
    @inbounds for d in 1:Nmin
        minlen, maxlen = minmax(A[d], B[d])
        minlen ≥ 1 || baddim(minlen)
        minlen == maxlen || minlen == 1 || baddims(minlen, maxlen)
        dims[d] = maxlen
    end
    @inbounds for d in Nmin+1:Nmax
        (len = B[d]) ≥ 1 || baddim(len)
        dims[d] = len
    end
    return (dims...,)
end

bcastdims(A::NTuple{N,Int}) where {N} = A

bcastdims(A::NTuple{Na,Int}, B::NTuple{Nb,Int}, args...) where {Na,Nb} =
    bcastdims(bcastdims(A, B), args...)
