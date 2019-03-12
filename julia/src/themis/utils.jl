#
# themis/utils.jl -
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

"""

```julia
pupilshape(args...) -> msk
```

yields a 2D mask (a Julia matrix of boolean elements) defining the
valid/active positions in a pupil.

"""
function pupilshape(rows::AbstractString...)
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

pupilshape(A::AbstractMatrix{Bool}) = A

"""

`Themis.WFS_SHAPE` is the mask (a 2D boolean array) defining the active
sub-pupils of the wavefront sensor with the central obscuration in Themis
adaptive optics system.  Without the central obscuration, use
[`Themis.WFS_FULL_SHAPE`](@ref).

"""
const WFS_SHAPE = pupilshape(
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
const WFS_FULL_SHAPE = pupilshape(
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
const DM_SHAPE = pupilshape(
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

function makegrid(pix1::NTuple{2,Real}, grd1::NTuple{2,Real},
                  pix2::NTuple{2,Real}, grd2::NTuple{2,Real};
                  shape::AbstractMatrix{Bool} = WFS_SHAPE)
    stp = (pix2 .- pix1)./(grd2 .- grd1)
    @printf("grid step: (%.3f,%.3f) pixels per grid node\n", stp...)
    avgstp = hypot(stp...)/sqrt(2)
    @printf("average grid step: %.3f pixels per grid node\n", avgstp)
    nsubs = count(shape)
    @printf("number of active sub-pupils: %d\n", nsubs)
    grid = Matrix{Cdouble}(undef, 2, nsubs)
    grd0 = (grd1 .+ grd2)./2
    pix0 = (pix1 .+ pix2)./2
    width, height = size(shape)
    k = 0
    for gy in 1:height
        for gx in 1:width
            if shape[gx,gy]
                k += 1
                grid[:,k] .= ((gx, gy) .- grd0).*avgstp .+ pix0
            end
        end
    end
    @assert k == nsubs
    return grid
end

"""

    Themis.loadconfig([T=Cdouble,] src) -> cfg

loads adaptive optics settings from source `src` (a filename or a FITS handle).
Optional argument `T` is to specify the floating-point type to use.

See also: [`Themis.load`](@ref), [`Themis.save`](@ref).

"""
loadconfig(src::Union{AbstractString,FITS}) = loadconfig(Cdouble, src)
loadconfig(::Type{T}, src::Union{AbstractString,FITS}) where {T<:AbstractFloat} =
    load(Configuration{T}, src)
