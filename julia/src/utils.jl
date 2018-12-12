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
arguments `A`, `B`, etc.  The result is a tuple of dimensions of type `Int`.
Call [`checkdims`](@ref) if you want to also make sure that the result is a
list of valid dimensions.

"""
bcastdims(::Tuple{}) = ()
bcastdims(a::Tuple{Vararg{Int}}) = a
bcastdims(a::Tuple{Vararg{Integer}}) = map(Int, a)
bcastdims(a::Tuple{Vararg{Integer}}, b::Tuple{Vararg{Integer}}, args...) =
    bcastdims(bcastdims(a, b), args...)

# Use a recursion to build the dimension list from two lists, if code is
# inlined (for a few number of dimensions), it should be much faster.
bcastdims(::Tuple{}, ::Tuple{}) = ()
bcastdims(::Tuple{}, b::Tuple{Vararg{Integer}}) = bcastdims(b)
bcastdims(a::Tuple{Vararg{Integer}}, ::Tuple{}) = bcastdims(a)
bcastdims(a::Tuple{Vararg{Integer}}, b::Tuple{Vararg{Integer}}) =
    (_bcastdim(a[1], b[1]), bcastdims(Base.tail(a), Base.tail(b))...)

# Apply broadcasting rules for a single dimension.
_bcastdim(a::Integer, b::Integer) = _bcastdim(Int(a), Int(b))
@static if false
    # Compared to Base.Broadcasting._bcs1, this version also checks the
    # validity of the dimensions (not just the compatibility). It is nearly as
    # fast (6.4 ns instead of 5.6 ns to build a list of 5 broadcasted
    # dimensions).
    _bcastdim(a::Int, b::Int) =
        (a == b || b == 1 ? (a ≥ 1 ? a : invalid_dimension()) :
         a == 1 ? (b ≥ 1 ? b : invalid_dimension()) :
         throw(DimensionMismatch("arrays could not be broadcast to a common size")))
else
    # This version does only checks the compatibility of dimensions and is as
    # fast as Base.Broadcasting._bcs1.
    _bcastdim(a::Int, b::Int) =
        (a == b || b == 1 ? a : a == 1 ? b :
         throw(DimensionMismatch("arrays could not be broadcast to a common size")))
end

"""

`checkdims(dims)` checks whether `dims` is a list of valid dimensions.

"""
checkdims(::Tuple{}) = true
checkdims(dims::Tuple{Vararg{Integer}}) =
    all(d -> d ≥ 1, dims) || error("invalid array dimension(s)")
