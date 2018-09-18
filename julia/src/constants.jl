#
# constants.jl --
#
# Definitions of constants for Julia interface to TAO.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO library (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

# Constant `taolib` is defined in another file which is built at installation
# time.
include("../deps/deps.jl")

"""

`TAO.SHARED_ANY` is the shared object type to use when any type is acceptable.

"""
const SHARED_ANY = Cint(-1)

"""

`TAO.SHARED_MAGIC` gives the 24 most significant bits in a TAO shared object
type.

"""
const SHARED_MAGIC = Cint(0x310efc00)

"""

`TAO.SHARED_OBJECT` is the type of a basic TAO shared object.

"""
const SHARED_OBJECT = Cint(SHARED_MAGIC | 0)

"""

`TAO.SHARED_ARRAY` is the type of a TAO shared multi-dimensional array.

"""
const SHARED_ARRAY = Cint(SHARED_MAGIC | 1)

"""

`TAO.SHARED_CAMERA` is the type of a TAO shared camera data.

"""
const SHARED_CAMERA = Cint(SHARED_MAGIC | 2)

"""

`TAO.SHARED_ARRAY_MAX_NDIMS` is the maximum number of dimensions of TAO shared
arrays.

"""
const SHARED_ARRAY_MAX_NDIMS = 5

const SharedArrayTypes = Union{Int8, UInt8, Int16, UInt16, Int32, UInt32,
                               Int64, UInt64, Float32, Float64}

const SHARED_ARRAY_TYPES = [Int8, UInt8, Int16, UInt16, Int32, UInt32,
                            Int64, UInt64, Float32, Float64]

const SHARED_ARRAY_ELTYPES = Dict{DataType,Cint}()
for i in eachindex(SHARED_ARRAY_TYPES)
    T = SHARED_ARRAY_TYPES[i]
    SHARED_ARRAY_ELTYPES[T] = Cint(i)
end
