#
# types.jl --
#
# Type definitions for Julia interface to the TAO C-library.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

"""

`TAO.LibTAO.Errors` is used to keep tracks of errors occuring in calls to TAO
library functions.

"""
mutable struct Errors # Must be mutable so that it can be passed by reference.
    ptr::Ptr{Cvoid}
    Errors() = new(C_NULL)
end

struct ErrorInfo
    func::String
    code::Cint
    proc::Ptr{Cvoid}
end

struct TaoError <: Exception
    errs::Array{ErrorInfo}
end
