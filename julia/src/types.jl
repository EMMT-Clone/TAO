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

Union `AnySharedObject` is defined to represent any TAO shared objects because
TAO shared arrays inherit from `DenseArray`, not from `AbstractSharedObject`.

"""
const AnySharedObject = Union{AbstractSharedObject,SharedArray}

struct TimeStamp
    sec::Int64
    nsec::Int64
end
