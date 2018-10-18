#
# types.jl --
#
# Type definitions for Julia interface to TAO.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

"""

`TAO.Errors` is used to keep tracks of errors occuring in calls to TAO
library functions.

"""
mutable struct Errors # Must be mutable so that it can be passed by reference.
    ptr::Ptr{Cvoid}
    Errors() = new(C_NULL)
end

struct ErrorInfo
    func::String
    code::Cint
end

struct TaoError <: Exception
    errs::Array{ErrorInfo}
end

abstract type AbstractSharedObject end

mutable struct SharedObject <: AbstractSharedObject
    ptr::Ptr{Cvoid}
    # Provide a unique inner constructor which forces starting with a NULL
    # pointer and no finalizer.
    SharedObject() = new(C_NULL)
end

"""

Type `TAO.SharedArray{T,N}` is a concrete subtype of `DenseArray{T,N}` which
includes all arrays where elements are stored contiguously in column-major
order.

"""
mutable struct SharedArray{T,N} <: DenseArray{T,N}
    ptr::Ptr{Cvoid}
    arr::Array{T,N}
end

"""

Type `TAO.SharedCamera` is used to represent shared camera data in Julia.

"""
mutable struct SharedCamera <: AbstractSharedObject
    ptr::Ptr{Cvoid}
    # Provide a unique inner constructor which forces starting with a NULL
    # pointer and no finalizer.
    SharedCamera() = new(C_NULL)
end

"""

Union `AnySharedObject` is defined to represent any TAO shared objects because
TAO shared arrays inherit from `DenseArray`, not from `AbstractSharedObject`.

"""
const AnySharedObject = Union{AbstractSharedObject,SharedArray}

const SHARED_OBJECT_TYPES = (AbstractSharedObject, SharedArray, SharedCamera)

struct TimeStamp
    sec::Int64
    nsec::Int64
end

"""

```julia
WeightedArray(wgt, dat, verify=false)
```

yields and instance of `WeightedArray` which stores an array of data `dat` with
corresponding weights `wgt`.  The conventions are that all weights and data
have finite values (to avoid numerical issues in computations) and that weights
are nonnegative (a weight of zero indicates an invalid data value).  If
optional argument `verify` is set true, theses assumptions are verified.

"""
struct WeightedArray{T<:AbstractFloat,N,W<:DenseArray{T,N},D<:DenseArray{T,N}}
    wgt::W
    dat::D
    # This unique inner constructor is to make sure that the two arrays have
    # the same indices.  There is however no way to prevent resizing the arrays
    # if they are vectors.
    function WeightedArray{T,N,W,D}(wgt::W, dat::D) where {
        W<:DenseArray{T,N},D<:DenseArray{T,N}} where {T<:AbstractFloat,N}
        axes(wgt) == axes(dat) || throw(DimensionMismatch("weights and data must have the same indices"))
        return new{T,N,W,D}(wgt, dat)
    end
end
