#
# utils.jl -
#
# Utilities for Themis adaptive optics software.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut and Michel Tallon.
#

function makeshape(rows::AbstractString...)
    nrows = length(rows)
    ncols = length(rows[1])
    A = Array{Bool,2}(undef, ncols, nrows)
    for y in 1:nrows
        row = rows[y]
        length(row) == ncols || error("all rows must have the same length")
        for x in 1:ncols
            A[x,y] = !isspace(row[x])
        end
    end
    return A
end

makeshape(A::AbstractMatrix{Bool}) = A

"""

`Themis.WFS_SHAPE` is the mask (a 2D boolean array) defining the active
sub-pupils of the wavefront sensor with the central obscuration in Themis
adaptive optics system.  Without the central obscuration, use
[`Themis.WFS_FULL_SHAPE`](@ref).

"""
const WFS_SHAPE = makeshape(
    "  xxxxxx  ",
    " xxxxxxxx ",
    "xxxxxxxxxx",
    "xxxx  xxxx",
    "xxx    xxx",
    "xxx    xxx",
    "xxxx  xxxx",
    "xxxxxxxxxx",
    " xxxxxxxx ",
    "  xxxxxx  ");

"""

`Themis.WFS_FULL_SHAPE` is the mask (a 2D boolean array) defining the active
sub-pupils of the wavefront sensor without the central obscuration in Themis
adaptive optics system.  With the central obscuration, use
[`Themis.WFS_SHAPE`](@ref).

"""
const WFS_FULL_SHAPE = makeshape(
    "  xxxxxx  ",
    " xxxxxxxx ",
    "xxxxxxxxxx",
    "xxxxxxxxxx",
    "xxxxxxxxxx",
    "xxxxxxxxxx",
    "xxxxxxxxxx",
    "xxxxxxxxxx",
    " xxxxxxxx ",
    "  xxxxxx  ");

"""

`Themis.DM_SHAPE` is the mask (a 2D boolean array) defining the layout of the
actuators of the deformable mirror of Themis adaptive optics system.

"""
const DM_SHAPE = makeshape(
    "   xxxxx   ",
    "  xxxxxxx  ",
    " xxxxxxxxx ",
    "xxxxxxxxxxx",
    "xxxxxxxxxxx",
    "xxxxxxxxxxx",
    "xxxxxxxxxxx",
    "xxxxxxxxxxx",
    " xxxxxxxxx ",
    "  xxxxxxx  ",
    "   xxxxx   ");

"""

    Themis.loadconfig([T=Cdouble,] src) -> cfg

loads adaptive optics settings from source `src` (a filename or a FITS handle).
Optional argument `T` is to specify the floating-point type to use.

See also: [`Themis.load`](@ref), [`Themis.save`](@ref).

"""
loadconfig(src::Union{AbstractString,FITS}) = loadconfig(Cdouble, src)
loadconfig(::Type{T}, src::Union{AbstractString,FITS}) where {T<:AbstractFloat} =
    load(Configuration{T}, src)
