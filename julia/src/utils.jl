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
bcastlazy(::Type{T}, dims::NTuple{N,Int}, A::DenseArray{T,N}) where {T,N} = A
bcastlazy(::Type{T}, dims::NTuple{N,Int}, A) where {T,N} =
    bcastcopy(T, dims, A)

"""

`bcastdims(size(A), size(B), ...)` yields the dimensions of the array that
would result from applying broadcasting rules (see [`broadcast`](@ref)) to
arguments `A`, `B`, etc.

"""
function bcastdims(A::NTuple{Na,Int}, B::NTuple{Nb,Int}) where {Na,Nb}
    if Na > Nb
        A, B = B, A
    end
    dims = Array{Int}(undef, Nb)
    for d in 1:Na
        A[d] ≥ 1 || @error "invalid dimension $(A[d])"
        B[d] ≥ 1 || @error "invalid dimension $(B[d])"
        if A[d] == B[d] || B[d] == 1
           dims[d] = A[d]
        elseif A[d] == 1
            dims[d] = B[d]
        else
            @error "incompatible dimensions $(A[d]) and $(B[d])"
        end
    end
    for i in Na+1:Nb
        B[d] ≥ 1 || @error "invalid dimension $(B[d])"
        dims[d] = B[d]
    end
    return (dims...,)
end

bcastdims(A::NTuple{N,Int}) where {N} = A

bcastdims(A::NTuple{Na,Int}, B::NTuple{Nb,Int}, args...)  where {Na,Nb} =
    bcastdims(bcastdims(A, B), args...)
